/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdint.h>

#include "base-gauge.h"
#include "data-source.h"
#include "generic-layer.h"
#include "map-gauge.h"
#include "map-math.h"
#include "map-tile-cache.h"
#include "map-provider.h"
#include "static-map-provider.h"
#include "route-map-provider.h"
#include "misc.h"
#include "sdl-colors.h"
#include "res-dirs.h"

#include "SDL_surface.h"
#include "SDL_timer.h"

/*Each tile is 256x256 px*/
#define TILE_SIZE 256
/*Time after which the viewport re-ties to the marker*/
#define MANIPULATE_TIMEOUT 2000
/* Scroll when the marker bouding box reaches this limit around the viewport*/
#define PIX_LIMIT 10

#define map_gauge_marker_left(self) ((self)->marker.x - generic_layer_w(&(self)->marker.layer)/2)
#define map_gauge_marker_top(self) ((self)->marker.y - generic_layer_h(&(self)->marker.layer)/2)

#define map_gauge_marker_worldbox(self) (SDL_Rect){ \
    .x = map_gauge_marker_left(self), \
    .y = map_gauge_marker_top(self), \
    .w = generic_layer_w(&(self)->marker.layer), \
    .h = generic_layer_h(&(self)->marker.layer) \
}


#define map_gauge_viewport(self) (SDL_Rect){ \
    .x = (self)->world_x, \
    .y = (self)->world_y, \
    .w = base_gauge_w(BASE_GAUGE((self))), \
    .h = base_gauge_h(BASE_GAUGE((self))) \
}


static void map_gauge_render(MapGauge *self, Uint32 dt, RenderContext *ctx);
static void map_gauge_update_state(MapGauge *self, Uint32 dt);
static MapGauge *map_gauge_dispose(MapGauge *self);
static BaseGaugeOps map_gauge_ops = {
   .render = (RenderFunc)map_gauge_render,
   .update_state = (StateUpdateFunc)map_gauge_update_state,
   .dispose = (DisposeFunc)map_gauge_dispose
};

/**
 * @brief Creates a new MapGauge of given dimensions.
 *
 * Caller is responsible for freeing the gauge by calling
 * base_gauge_free.
 *
 * @param w Width (in pixels) of the gauge
 * @param h Height (in pixels) of the gauge
 * @return a newly-allocated MapGauge on success, NULL on failure.
 *
 * @see base_gauge_free
 */
MapGauge *map_gauge_new(int w, int h)
{
    MapGauge *rv;

    rv = calloc(1, sizeof(MapGauge));
    if(rv){
        if(!map_gauge_init(rv,w,h))
            return base_gauge_free(BASE_GAUGE(rv));
    }
    return rv;
}

/**
 * @brief Inits an already allocated MapGauge with given dimensions.
 *
 * This function must not be called twice on the same object
 * without calling map_gauge_dispose inbetween.
 *
 * @param self a MapGauge
 * @param w Width (in pixels) of the gauge
 * @param h Height (in pixels) of the gauge
 * @return @p self on success, NULL on failure.
 *
 * @see map_gauge_dispose
 * @see map_gauge_new
 */
MapGauge *map_gauge_init(MapGauge *self, int w, int h)
{
    int twidth, theight;
    size_t cache_tiles;

    base_gauge_init(BASE_GAUGE(self),
        &map_gauge_ops,
        w, h
    );

    twidth = w / TILE_SIZE;
    theight = h /TILE_SIZE;
    /* Worst case is the view centered on the junction of 4 tiles
     * multiplied by the number of tiles the view can see at once.
     * with a minimum of 1 if the view is smaller than a tile
     */
    /*Keep in the tile stack 2 viewports worth of tiles*/
    cache_tiles = (MAX(twidth, 1) * MAX(twidth, 1)) * 4;
    map_tile_cache_init(&self->tile_cache, cache_tiles);

    /*TODO: Runtime / GUI selection of maps*/
#if HAVE_IGN_OACI_MAP
    self->tile_providers[self->ntile_providers++] = (MapProvider*)static_map_provider_new(
        MAPS_HOME"/ign-oaci", "jpg",0
    );
#else
    self->tile_providers[self->ntile_providers++] = (MapProvider*)static_map_provider_new(
        MAPS_HOME"/osm", "png", 0
    );
#endif

#if !HAVE_IGN_OACI_MAP
    self->overlays[self->noverlays++] = (MapProvider*)static_map_provider_new(
        MAPS_HOME"/openaip", "png", 0
    );
#endif

    self->route_overlay = route_map_provider_new();
    if(!self->route_overlay)
        return NULL;

    qsort(self->tile_providers,
        self->ntile_providers,
        sizeof(MapProvider*), (__compar_fn_t)map_provider_compare_ptr
    );
    qsort(self->overlays,
        self->noverlays,
        sizeof(MapProvider*), (__compar_fn_t)map_provider_compare_ptr
    );


    /*TODO: Scale the plane relative to the gauge's size*/
    generic_layer_init_from_file(&self->marker.layer, IMG_DIR"/plane32.png");
    generic_layer_build_texture(&self->marker.layer);

    return self;
}

/**
 * @brief Release any resources internally held by the MapGauge
 *
 * This function always returns NULL (convenience behavior).
 *
 * @param self a MapGauge
 * @return NULL
 */
static MapGauge *map_gauge_dispose(MapGauge *self)
{
    for(int i = 0; i < self->state.npatches; i++)
        generic_layer_unref(self->state.patches[i].layer);
    if(self->state.patches)
        free(self->state.patches);

    generic_layer_dispose(&self->marker.layer);
    for(int i = 0; i < self->ntile_providers; i++)
        map_provider_free(self->tile_providers[i]);

    for(int i = 0; i < self->noverlays; i++)
        map_provider_free(self->overlays[i]);

    map_provider_free(MAP_PROVIDER(self->route_overlay));
    map_tile_cache_dispose(&self->tile_cache);
    return self;
}

/**
 * @brief Sets the current zoom level show by the gauge. Valid levels are
 * 0 to MAP_GAUGE_MAX_LEVEL, owing to types internally used to store positions.

 * The current maximum level is 23 with ints (32 bits) which is more than
 * sufficient for aviation purposes
 *
 * This function will try its best to keep the current area and zoom on it.
 *
 * @param self a MapGauge
 * @param level the level to show
 * @return true on success, false on failure (level unsupported, ...)
 */
bool map_gauge_set_level(MapGauge *self, uintf8_t level)
{
    double lat, lon;
    int32_t new_x, new_y;

    if(level > 15)
        return false;
    if(level != self->level){
        /* Keep the view at the same place. TODO: There should be a way to do the
         * same without having to go through geo coords transforms*/
        map_math_pixel_to_geo(self->world_x, self->world_y, self->level, &lat, &lon);
        map_math_geo_to_pixel(lat, lon, level, &new_x, &new_y);
        /*Same for the marker*/
        map_math_pixel_to_geo(self->marker.x, self->marker.y, self->level, &lat, &lon);
        self->level = level;
        map_gauge_set_viewport(self, new_x, new_y, false);
        map_gauge_set_marker_position(self, lat, lon);
    }
    return true;
}

/**
 * @brief Updates the marker position
 *
 * Client code should use this function to make the marker move.
 *
 * @param self a MapGauge
 * @param latitude The new latitude of the marker
 * @param longitude The new longitude of the marker
 * @return true on success, false on failure.
 */
bool map_gauge_set_marker_position(MapGauge *self, double latitude, double longitude)
{
    bool rv;
    int32_t new_x,new_y;

    /* TODO: This is purely based on time and should not be in this function
     * it should be some kind of animation or use another system to have
     * time-based events
     * */
    if(self->roaming && SDL_GetTicks() - self->last_manipulation > MANIPULATE_TIMEOUT){
        self->roaming = false;
        map_gauge_center_on_marker(self, true);
    }

    rv = map_math_geo_to_pixel(latitude, longitude, self->level, &new_x, &new_y);
    if(new_x != self->marker.x || new_y != self->marker.y){
        self->marker.x = new_x;
        self->marker.y = new_y;
        if(!self->roaming){
            map_gauge_follow_marker(self);
        }
        BASE_GAUGE(self)->dirty = true;
        return true;
    }
    return false;
}

/**
 * @brief Updates the marker heading (dregrees, 0-360). If the value
 * is outside the valid range, it will be clamped to it.
 *
 * Client code should use this function to make the marker face
 * the direction it's heading towards.
 *
 * @param self a MapGauge
 * @param heading The new heading of the marker, in degrees
 * @return true on success, false on failure.
 */
bool map_gauge_set_marker_heading(MapGauge *self, float heading)
{
    heading = clampf(heading, 0, 360);
    if(heading != self->marker.heading){
        self->marker.heading = heading;
        BASE_GAUGE(self)->dirty = true;
        return true;
    }
    return false;
}

/**
 * @brief Moves the viewport by the given increment while putting it in a
 * temporary 'roaming' mode. Roaming mode will last MANIPULATE_TIMEOUT ms
 * after the last call to map_gauge_manipulate_viewport.
 *
 * While in roaming mode, the viewport is free to roam the entire map without
 * being dragged back when the marker moves. Once the roaming mode expires, the
 * viewport automatically goes back centered on the marker position.
 *
 * This function is intended to be called by 'client' code.
 *
 * @param self a MapGauge
 * @param dx increment for the x position, positive or negative
 * @param dy increment for the y position, positive or negative
 * @param animated When true, will show a nice transition from the current
 * viewport position to the aforementioned area.
 * @return true on success, false on failure.
 */
bool map_gauge_manipulate_viewport(MapGauge *self, int32_t dx, int32_t dy, bool animated)
{
    self->last_manipulation = SDL_GetTicks();
    self->roaming = true;
    return map_gauge_set_viewport(self,
            self->world_x + dx,
            self->world_y + dy,
            animated
    );
}

/**
 * @brief Resets the viewport to show the area surrounding the marker
 * with the marker in the center.
 *
 * Client code can use this function to reset the view on command. Doing
 * so is not mandatory as the default behavior of the gauge is to go back
 * to a marker-on-center position when:
 * -Roaming mode expores, @see map_gauge_manipulate_viewport
 * -The marker reaches the edges of the current viewport
 *
 * @param self a MapGauge
 * @param animated When true, will show a nice transition from the current
 * viewport position to the aforementioned area.
 * @return true on success, false on failure.
 */
bool map_gauge_center_on_marker(MapGauge *self, bool animated)
{
    return map_gauge_set_viewport(self,
            map_gauge_marker_left(self) - base_gauge_center_x(BASE_GAUGE(self)),
            map_gauge_marker_top(self) - base_gauge_center_y(BASE_GAUGE(self)),
            animated
    );
}

/**
 * @brief Move the viewport according to the current marker position
 *
 * Mainly internal function
 *
 * @param self a MapGauge
 * @return true on success, false on failure
 */
bool map_gauge_follow_marker(MapGauge *self)
{
    bool visible;

    visible = SDL_IntersectRect(&map_gauge_viewport(self),
        &map_gauge_marker_worldbox(self),
        &self->state.marker_src
    );
    if(!visible){
        return map_gauge_center_on_marker(self, true);
    }

    /*marker_x and marker_y are top left coordinates (world)*/
    if( map_gauge_marker_left(self) <= self->world_x + PIX_LIMIT
        || map_gauge_marker_left(self) + generic_layer_w(&self->marker.layer) >= self->world_x + base_gauge_w(BASE_GAUGE(self)) - PIX_LIMIT
        || map_gauge_marker_top(self) <= self->world_y + PIX_LIMIT
        || map_gauge_marker_top(self) + generic_layer_h(&self->marker.layer) >= self->world_x + base_gauge_h(BASE_GAUGE(self)) - PIX_LIMIT
    )
        return map_gauge_center_on_marker(self, true);
    return true;
}

/**
 * @brief Moves the viewport by the given increment (in pixels).
 *
 * Mainly for interal use, might not be the function you are looking for.
 * @see map_gauge_manipulate_viewport
 *
 * @param self a MapGauge
 * @param dx increment for the x position, positive or negative
 * @param dy increment for the y position, positive or negative
 * @param animated When true, will show a nice transition from the current
 * viewport position to the aforementioned area.
 * @return true on success, false on failure.
 */
bool map_gauge_move_viewport(MapGauge *self, int32_t dx, int32_t dy, bool animated)
{
    return map_gauge_set_viewport(self,
            self->world_x + dx,
            self->world_y + dy,
            animated
    );
}

/**
 * @brief Sets the viewport to the given position (in pixels). The position
 * is a "world" position in the virtual current map level that goes from
 * 0,0 to (256*2^level)-1,(256*2^level)-1.
 *
 * This function takes an absolute position to go to. For a movement relative
 * to the current position, @see map_gauge_move_viewport.
 *
 * Mainly for interal use, might not be the function you are looking for.
 * @see map_gauge_manipulate_viewport
 *
 * @param self a MapGauge
 * @param x The new absolute x position
 * @param y The new absolute y position
 * @param animated When true, will show a nice transition from the current
 * viewport position to the aforementioned area.
 * @return true on success, false on failure.
 */
bool map_gauge_set_viewport(MapGauge *self, int32_t x, int32_t y, bool animated)
{
    uint32_t map_lastcoord = map_math_size(self->level) - 1;
    x = clamp(x, 0, map_lastcoord - base_gauge_w(BASE_GAUGE(self)));
    y = clamp(y, 0, map_lastcoord - base_gauge_h(BASE_GAUGE(self)));

    if(x == self->world_x && y == self->world_y)
        return false;

    animated = false;
    if(animated){
    /*start an animation that moves current coords to their
     * destination values*/
    }else{
        self->world_x = x;
        self->world_y = y;
        BASE_GAUGE(self)->dirty = true;
    }
    return true;
}

static GenericLayer *map_gauge_get_tile(MapGauge *self, uintf8_t level, int32_t x, int32_t y)
{
    GenericLayer *rv;

    rv = map_tile_cache_get(&self->tile_cache, level, x, y);
    if(rv)
        return rv;

    /* Cache miss, get tile from providers and cache it
     *
     * Providers are sorted by priority. The first provider
     * to respond will have its tile used. If the a provider
     * priority is negative, no overlay will be applied
     * */
    for(int i = 0; i < self->ntile_providers; i++){
        rv = map_provider_get_tile(self->tile_providers[i],
            level, x, y
        );
        if(rv){
            if(self->tile_providers[i]->priority < 0 )
                goto end;
            break;
        }
    }
    if(!rv) return NULL;//No-one has the tile, bail out
    /* If the selected provider wasn't a negative priority
     * provider, apply all overlays on the tile
     * */
    GenericLayer *tmp;
    for(int i = 0; i < self->noverlays; i++){
        tmp = map_provider_get_tile(self->overlays[i], level, x, y);
        if(!tmp) continue;
        SDL_BlitSurface(
            tmp->canvas, NULL,
            rv->canvas,NULL
        );
        generic_layer_free(tmp);
    }

    /*Apply the route drawing, if any*/
    tmp = map_provider_get_tile(MAP_PROVIDER(self->route_overlay), level, x, y);
    if(tmp){
        SDL_BlitSurface(
            tmp->canvas, NULL,
            rv->canvas,NULL
        );
        generic_layer_free(tmp);
    }
end:
    generic_layer_build_texture(rv);
    map_tile_cache_add(&self->tile_cache, rv, level, x, y);
    return rv;
}

void map_gauge_location_changed(MapGauge *self, LocationData *newv)
{
    map_gauge_set_marker_position(self, newv->super.latitude, newv->super.longitude);
}

void map_gauge_attitude_changed(MapGauge *self, AttitudeData *newv)
{
    map_gauge_set_marker_heading(self, newv->heading);
}

void map_gauge_route_changed(MapGauge *self, RouteData *newv)
{
    route_map_provider_set_route(self->route_overlay,
        &newv->from, &newv->to
    );
    map_tile_cache_clear(&self->tile_cache);
    BASE_GAUGE(self)->dirty = true;
}

/*TODO: split up*/
static void map_gauge_update_state(MapGauge *self, Uint32 dt)
{
    /* We go up to level 23, which is 8388608 tiles
     * (from 0 to 8388607) in both directions*/
    int32_t tl_tile_x, tl_tile_y; /*top left*/
    int32_t br_tile_x, br_tile_y; /*bottom right*/

    tl_tile_x = self->world_x / TILE_SIZE;
    tl_tile_y = self->world_y / TILE_SIZE;

    int32_t lastx = self->world_x + base_gauge_w(BASE_GAUGE(self)) - 1;
    int32_t lasty = self->world_y + base_gauge_h(BASE_GAUGE(self)) - 1;
    br_tile_x = lastx / TILE_SIZE;
    br_tile_y = lasty / TILE_SIZE;

    int32_t x_tile_span = (br_tile_x - tl_tile_x) + 1;
    int32_t y_tile_span = (br_tile_y - tl_tile_y) + 1;
    int32_t tile_span = x_tile_span * y_tile_span;

    /*There will be as many patches as tiles over which we are located*/
    /* Currently an X,Y tile is the fusion of all X,Y tiles
     * given by providers that can provide that tile.
     */
    if(tile_span > self->state.apatches){
        void *tmp;
        size_t stmp;
        stmp = self->state.apatches;
        self->state.apatches = tile_span;
        tmp = realloc(self->state.patches, self->state.apatches*sizeof(MapPatch));
        if(!tmp){
            self->state.apatches = stmp;
            return;
        }
        self->state.patches = tmp;
    }

    for(int i = 0; i < self->state.npatches; i++)
        generic_layer_unref(self->state.patches[i].layer);
    self->state.npatches = 0;

    GenericLayer *layer;
    SDL_Rect viewport = map_gauge_viewport(self);
    for(int tiley = tl_tile_y; tiley <= br_tile_y; tiley++){
        for(int tilex = tl_tile_x; tilex <= br_tile_x; tilex++){
            layer = map_gauge_get_tile(self, self->level, tilex, tiley);
            if(!layer)
                printf("Couldn't get tile layer for tile x:%d y:%d zoom:%d\n",tilex,tiley, self->level);
            if(!layer) continue;
            /*TODO: Use rects with uint32_t,
             * SDL uses ints and will only go up to level 15*/
            SDL_Rect tile = {
                .x = TILE_SIZE * tilex,
                .y = TILE_SIZE * tiley,
                .w = TILE_SIZE,
                .h = TILE_SIZE
            };
            /*Get intersection of the tile with the viewport, in world coordinates*/
            SDL_IntersectRect(&viewport,
                &tile,
                &self->state.patches[self->state.npatches].src
            );
            self->state.patches[self->state.npatches].dst = self->state.patches[self->state.npatches].src;
            /*Change src to be in the tile's local coordinates (0-255)*/
            self->state.patches[self->state.npatches].src.x -= tile.x;
            self->state.patches[self->state.npatches].src.y -= tile.y;
            /*Change dst to be in the viewport's local coordinates (0-(w-1),0-(h-1)*/
            self->state.patches[self->state.npatches].dst.x -= self->world_x;
            self->state.patches[self->state.npatches].dst.y -= self->world_y;

            self->state.patches[self->state.npatches].layer = layer;
            generic_layer_ref(layer);
            self->state.npatches++;
        }
    }

    /*Get intersection of the marker with the viewport, in world coordinates*/
    bool marker_visible = SDL_IntersectRect(&viewport,
        &map_gauge_marker_worldbox(self),
        &self->state.marker_src
    );
    if(marker_visible){
        self->state.marker_dst = self->state.marker_src;
        /*Change src to be in the marker's local coordinates (0-(w-1),0-(h-1)*/
        self->state.marker_src.x -= map_gauge_marker_left(self);
        self->state.marker_src.y -= map_gauge_marker_top(self);
        /*Change dst to be in the viewport's local coordinates (0-(w-1),0-(h-1)*/
        self->state.marker_dst.x -= self->world_x;
        self->state.marker_dst.y -= self->world_y;
    }else{
        self->state.marker_dst = (SDL_Rect){-1,-1,-1,-1};
        self->state.marker_src = self->state.marker_dst;
    }
}

static void map_gauge_render(MapGauge *self, Uint32 dt, RenderContext *ctx)
{
    MapPatch *patch;


    for(int i = 0; i < self->state.npatches; i++){
        patch = &self->state.patches[i];
        base_gauge_blit_layer(BASE_GAUGE(self), ctx,
            patch->layer, &patch->src,
            &patch->dst
        );
    }
    if(self->state.marker_src.x >= 0){
#if 0
        base_gauge_blit_layer(BASE_GAUGE(self), ctx,
            &self->marker,
            &self->state.marker_src,
            &self->state.marker_dst
        );
#endif
        base_gauge_blit_rotated_texture(BASE_GAUGE(self), ctx,
            self->marker.layer.texture, &self->state.marker_src,
            self->marker.heading,
            NULL,
            &self->state.marker_dst,
            NULL);
    }
    base_gauge_draw_outline(BASE_GAUGE(self), ctx, &SDL_WHITE, NULL);
}

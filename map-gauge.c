#include <stdint.h>

#include "SDL_surface.h"
#include "base-gauge.h"
#include "generic-layer.h"
#include "map-gauge.h"
#include "map-math.h"
#include "map-tile-provider.h"
#include "misc.h"
#include "sdl-colors.h"

/*Each tile is 256x256 px*/
#define TILE_SIZE 256

static void map_gauge_render(MapGauge *self, Uint32 dt, RenderContext *ctx);
static void map_gauge_update_state(MapGauge *self, Uint32 dt);
static BaseGaugeOps map_gauge_ops = {
   .render = (RenderFunc)map_gauge_render,
   .update_state = (StateUpdateFunc)map_gauge_update_state
};

MapGauge *map_gauge_new(int w, int h)
{
    MapGauge *rv;

    rv = calloc(1, sizeof(MapGauge));
    if(rv){
        if(!map_gauge_init(rv,w,h))
            return map_gauge_free(rv);
    }
    return rv;
}

MapGauge *map_gauge_init(MapGauge *self, int w, int h)
{
    int twidth, theight;
    size_t cache_tiles;

    twidth = w / TILE_SIZE;
    theight = h /TILE_SIZE;
    /* Worst case is the view centered on the junction of 4 tiles
     * multiplied by the number of tiles the view can see at once.
     * with a minimum of 1 if the view is smaller than a tile
     */
    cache_tiles = (MAX(twidth, 1) * MAX(twidth, 1)) * 4;

    /*Keep in the tile stack 2 viewports worth of tiles*/
    self->tile_provider = map_tile_provider_new(cache_tiles*2);
    /*TODO: Scale the plane relative to the gauge's size*/
    generic_layer_init_from_file(&self->marker, "plane32.png");
    generic_layer_build_texture(&self->marker);

    base_gauge_init(BASE_GAUGE(self),
        &map_gauge_ops,
        w, h
    );

    return self;
}

MapGauge *map_gauge_dispose(MapGauge *self)
{
    if(self->tile_provider)
        map_tile_provider_free(self->tile_provider);
    base_gauge_dispose(BASE_GAUGE(self));
    return NULL;
}

MapGauge *map_gauge_free(MapGauge *self)
{
    map_gauge_dispose(self);
    free(self);
    return NULL;
}

bool map_gauge_set_level(MapGauge *self, uintf8_t level)
{
    double lat, lon;
    uint32_t new_x, new_y;

    if(level > 15)
        return false;
    if(level != self->level){
        /* Keep the view at the same place. TODO: There should be a way to do the
         * same without having to go through geo coords transforms*/
        map_math_pixel_to_geo(self->world_x, self->world_y, self->level, &lat, &lon);
        map_math_geo_to_pixel(lat, lon, level, &new_x, &new_y);
        /*Same for the marker*/
        map_math_pixel_to_geo(self->marker_x, self->marker_y, self->level, &lat, &lon);
        self->level = level;
        map_gauge_set_viewport(self, new_x, new_y, false);
        map_gauge_set_marker_position(self, lat, lon);
    }
    return true;
}

bool map_gauge_set_marker_position(MapGauge *self, double latitude, double longitude)
{
    bool rv;
    uint32_t new_x,new_y;
    rv = map_math_geo_to_pixel(latitude, longitude, self->level, &new_x, &new_y);
    if(new_x != self->marker_x || new_y != self->marker_y){
        self->marker_x = new_x;
        self->marker_y = new_y;
        BASE_GAUGE(self)->dirty = true;
        return true;
    }
    return false;
}

bool map_gauge_center_on_marker(MapGauge *self, bool animated)
{
    return map_gauge_set_viewport(self,
            self->marker_x - base_gauge_center_x(BASE_GAUGE(self)),
            self->marker_y - base_gauge_center_y(BASE_GAUGE(self)),
            animated
    );
}

bool map_gauge_move_viewport(MapGauge *self, int32_t dx, int32_t dy, bool animated)
{
    return map_gauge_set_viewport(self,
            self->world_x + dx,
            self->world_y + dy,
            animated
    );
}

bool map_gauge_set_viewport(MapGauge *self, uint32_t x, uint32_t y, bool animated)
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

static void map_gauge_update_state(MapGauge *self, Uint32 dt)
{
    /* We go up to level 16, which is 65536 tiles
     * (from 0 to 65535) in both directions*/
    uintf16_t tl_tile_x, tl_tile_y; /*top left*/
    uintf16_t br_tile_x, br_tile_y; /*bottom right*/

    tl_tile_x = self->world_x / TILE_SIZE;
    tl_tile_y = self->world_y / TILE_SIZE;

    uint32_t lastx = self->world_x + base_gauge_w(BASE_GAUGE(self)) - 1;
    uint32_t lasty = self->world_y + base_gauge_h(BASE_GAUGE(self)) - 1;
    br_tile_x = lastx / TILE_SIZE;
    br_tile_y = lasty / TILE_SIZE;

    uintf16_t x_tile_span = (br_tile_x - tl_tile_x) + 1;
    uintf16_t y_tile_span = (br_tile_y - tl_tile_y) + 1;
    uintf16_t tile_span = x_tile_span * y_tile_span;

    /*There will be as many patches as tiles over which we are located*/
    /*TODO: Multiply by the number of providers*/
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
    SDL_Rect viewport = (SDL_Rect){
        .x = self->world_x,
        .y = self->world_y,
        .w = base_gauge_w(BASE_GAUGE(self)),
        .h = base_gauge_h(BASE_GAUGE(self))
    };
    for(int tiley = tl_tile_y; tiley <= br_tile_y; tiley++){
        for(int tilex = tl_tile_x; tilex <= br_tile_x; tilex++){
            layer = map_tile_provider_get_tile(self->tile_provider, self->level, tilex, tiley);
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

    SDL_Rect marker_world = {
        .x = self->marker_x,
        .y = self->marker_y,
        .w = generic_layer_w(&self->marker),
        .h = generic_layer_h(&self->marker)
    };
    /*Get intersection of the marker with the viewport, in world coordinates*/
    bool marker_visible = SDL_IntersectRect(&viewport,
        &marker_world,
        &self->state.marker_src
    );
    if(marker_visible){
        self->state.marker_dst = self->state.marker_src;
        /*Change src to be in the marker's local coordinates (0-(w-1),0-(h-1)*/
        self->state.marker_src.x -= self->marker_x;
        self->state.marker_src.y -= self->marker_y;
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
//        if(i != 0) continue;
        patch = &self->state.patches[i];
        base_gauge_blit_layer(BASE_GAUGE(self), ctx,
            patch->layer, &patch->src,
            &patch->dst
        );
    }
    if(self->state.marker_src.x >= 0){
        base_gauge_blit_layer(BASE_GAUGE(self), ctx,
            &self->marker,
            &self->state.marker_src,
            &self->state.marker_dst
        );
    }
    base_gauge_draw_outline(BASE_GAUGE(self), ctx, &SDL_WHITE, NULL);
}




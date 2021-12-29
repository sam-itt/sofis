/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include "generic-layer.h"
#include "route-map-provider.h"
#include "geo-location.h"
#include "map-provider.h"
#include "misc.h"
#include "map-math.h"
#include "sdl-colors.h"

/*Line drawing adapted from https://github.com/miloyip/line*/
static float capsuleSDF(float px, float py, float ax, float ay, float bx, float by, float r);
static void line_portion(GenericLayer *layer, int tx, int ty,
                  SDL_Point world_from,
                  SDL_Point world_to,
                  int r,
                  SDL_Color color);
static inline void alphablend(GenericLayer *layer, int x, int y,
                Uint8 alpha, Uint8 r, Uint8 g, Uint8 b);

static GenericLayer *route_map_provider_get_tile(RouteMapProvider *self,
                                                 uintf8_t level,
                                                 int32_t x, int32_t y);

static MapProviderOps route_map_provider_ops = {
    .get_tile = (MapProviderGetTileFunc)route_map_provider_get_tile,
    .dispose = NULL
};

RouteMapProvider *route_map_provider_new(void)
{
    RouteMapProvider *self;

    self = calloc(1, sizeof(RouteMapProvider));
    if(self){
        if(!route_map_provider_init(self))
            return (RouteMapProvider*)map_provider_free(MAP_PROVIDER(self));
    }
    return self;
}

RouteMapProvider *route_map_provider_init(RouteMapProvider *self)
{
    map_provider_init(MAP_PROVIDER(self), &route_map_provider_ops, 10);
    map_provider_set_nareas(MAP_PROVIDER(self), 1); /*TODO: Inefficient*/
    self->current_zoom = -1;
    self->geo_from.latitude = NAN;
    return self;
}

bool route_map_provider_set_route(RouteMapProvider *self,
                                  GeoLocation *from,
                                  GeoLocation *to)
{
    self->geo_from = *from;
    self->geo_to = *to;
    self->current_zoom = -1;

    return true;
}

static bool route_map_provider_set_level(RouteMapProvider *self, uintf8_t level)
{
    self->current_zoom = level;

    map_math_geo_to_pixel(
        self->geo_from.latitude, self->geo_from.longitude,
        level, &self->from.x, &self->from.y
    );
    map_math_geo_to_pixel(
        self->geo_to.latitude, self->geo_to.longitude,
        level, &self->to.x, &self->to.y
    );
    MAP_PROVIDER(self)->areas[0] = (MapProviderArea){
        .top = MIN(self->from.y, self->to.y)/256,
        .left = MIN(self->from.x, self->to.x)/256,
        .bottom = MAX(self->from.y, self->to.y)/256,
        .right = MAX(self->from.x, self->to.x)/256,
        .level = level
    };

    return true;
}

static GenericLayer *route_map_provider_get_tile(RouteMapProvider *self,
                                                 uintf8_t level,
                                                 int32_t x, int32_t y)
{
    GenericLayer *rv = NULL;

    if(isnan(self->geo_from.latitude)) return NULL;

    if(self->current_zoom != level)
        route_map_provider_set_level(self, level);

    if(MAP_PROVIDER(self)->nareas && !map_provider_has_tile(MAP_PROVIDER(self), level, x, y))
        return NULL;

    rv = generic_layer_new(256, 256);

    line_portion(rv, x, y, self->from, self->to, 2, SDL_RED);

    return rv;
}

static float capsuleSDF(float px, float py, float ax, float ay, float bx, float by, float r)
{
    float pax = px - ax, pay = py - ay, bax = bx - ax, bay = by - ay;
    float h = fmaxf(fminf((pax * bax + pay * bay) / (bax * bax + bay * bay), 1.0f), 0.0f);
    float dx = pax - bax * h, dy = pay - bay * h;
    return sqrtf(dx * dx + dy * dy) - r;
}

static inline void alphablend(GenericLayer *layer, int x, int y,
                Uint8 alpha, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Surface *output = layer->canvas;

    if(x < 0 || y < 0 || x > output->w-1 || y > output->h-1 ){
        return;
    }
    uint8_t *p = (Uint8*)output->pixels
               + (y * output->pitch)
               + (x * output->format->BytesPerPixel);
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = alpha;
}

static void line_portion(GenericLayer *layer, int tx, int ty,
                  SDL_Point world_from,
                  SDL_Point world_to,
                  int r,
                  SDL_Color color)
{
    /*Tile 0: 0 -> 255 Tile 1: 256 -> 511*/
    int ax = tx * 256;
    int bx = ax + 256 - 1;
    int ay = ty * 256;
    int by = ay + 256 - 1;

    int x0 = MIN(ax, bx) - r;
    int x1 = MAX(ax, bx) + r;
    int y0 = MIN(ay, by) - r;
    int y1 = MAX(ay, by) + r;

    generic_layer_lock(layer);
    for (int y = y0; y <= y1; y++){
        for (int x = x0; x <= x1; x++){
            float alpha =fmaxf(
                fminf(0.5f - capsuleSDF(
                                x,
                                y,
                                world_from.x, world_from.y,
                                world_to.x, world_to.y,
                                r
                            ),
                       1.0f
                ),
                0.0f
            );

            alphablend(layer,
                x - tx * 256,
                y - ty * 256,
                (Uint8)(alpha * 255),
                color.r, color.g, color.b
            );
        }
    }
    generic_layer_unlock(layer);
}


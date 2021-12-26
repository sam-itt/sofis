/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MAP_GAUGE_H
#define MAP_GAUGE_H

#include "base-gauge.h"
#include "generic-layer.h"
#include "map-tile-cache.h"
#include "map-provider.h"
#include "route-map-provider.h"
#include "data-source.h"
#include "misc.h"

/* int(32) pixel coordinates go up to
 * 2,147,483,647 which is the maximum coordinate
 * of level 23
 */
#define MAP_GAUGE_MAX_LEVEL 23

typedef struct{
    /*TODO: Array of pointers to layers, as much as providers/overlays*/
    GenericLayer *layer;
    SDL_Rect src;
    SDL_Rect dst;
}MapPatch;

typedef struct{
    MapPatch *patches;
    size_t apatches;
    size_t npatches;

    SDL_Rect marker_src;
    SDL_Rect marker_dst;
}MapGaugeState;

typedef struct{
    GenericLayer layer;
    /* Marker position in "world" coordinates
     * world coordinates are respective to the
     * current level.
     *
     * Note: this is the point marked, the icon
     * itself is centered on these coordinates
     */
    int32_t x;
    int32_t y;
    float heading;
}MapGaugeMarker;

typedef struct{
    BaseGauge super;

    MapTileCache tile_cache;
    /*current zoom level*/
    uintf8_t level;
    /*Top-left coordinates of the viewport*/
    int32_t world_x;
    int32_t world_y;

    /*The little plane on the map*/
    MapGaugeMarker marker;

    bool roaming; /*The view is roaming around and not tied to the marker*/
    Uint32 last_manipulation;

    MapProvider *tile_providers[2]; /*static for now*/
    size_t ntile_providers;

    MapProvider *overlays[1]; /*static for now*/
    size_t noverlays;

    RouteMapProvider *route_overlay;

    MapGaugeState state;
}MapGauge;

MapGauge *map_gauge_new(int w, int h);
MapGauge *map_gauge_init(MapGauge *self, int w, int h);

bool map_gauge_set_level(MapGauge *self, uintf8_t level);
bool map_gauge_set_marker_position(MapGauge *self, double latitude, double longitude);
bool map_gauge_set_marker_heading(MapGauge *self, float heading);
bool map_gauge_manipulate_viewport(MapGauge *self, int32_t dx, int32_t dy, bool animated);
bool map_gauge_center_on_marker(MapGauge *self, bool animated);

bool map_gauge_follow_marker(MapGauge *self);
bool map_gauge_move_viewport(MapGauge *self, int32_t dx, int32_t dy, bool animated);
bool map_gauge_set_viewport(MapGauge *self, int32_t x, int32_t y, bool animated);


void map_gauge_location_changed(MapGauge *self, LocationData *newv);
void map_gauge_attitude_changed(MapGauge *self, AttitudeData *newv);
#endif /* MAP_GAUGE_H */

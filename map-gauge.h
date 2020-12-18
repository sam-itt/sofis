#ifndef MAP_GAUGE_H
#define MAP_GAUGE_H

#include "base-gauge.h"
#include "generic-layer.h"
#include "map-tile-provider.h"
#include "misc.h"

/*With uint32_t for coords we
 * can go to the 16th level*/
#define MAP_GAUGE_MAX_LEVEL 16

typedef struct{
    GenericLayer *layer; /*TODO: Array of pointers to layers, as much as providers*/
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
    BaseGauge super;

    /*current zoom level*/
    uintf8_t level;
    /*Top-left coordinates of the viewport*/
    uint32_t world_x;
    uint32_t world_y;

    /*The little plane on the map*/
    GenericLayer marker;
    /* Marker position in "world" coordinates
     * world coordinates are respective to the
     * current level.
     */
    uint32_t marker_x;
    uint32_t marker_y;

    MapTileProvider *tile_provider;
    MapGaugeState state;
}MapGauge;

MapGauge *map_gauge_new(int w, int h);
MapGauge *map_gauge_init(MapGauge *self, int w, int h);


MapGauge *map_gauge_dispose(MapGauge *self);
MapGauge *map_gauge_free(MapGauge *self);


bool map_gauge_set_level(MapGauge *self, uintf8_t level);
bool map_gauge_set_marker_position(MapGauge *self, double latitude, double longitude);
bool map_gauge_center_on_marker(MapGauge *self, bool animated);
bool map_gauge_move_viewport(MapGauge *self, int32_t dx, int32_t dy, bool animated);
bool map_gauge_set_viewport(MapGauge *self, uint32_t x, uint32_t y, bool animated);
#endif /* MAP_GAUGE_H */

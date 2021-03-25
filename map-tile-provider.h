#ifndef MAP_TILE_PROVIDER_H
#define MAP_TILE_PROVIDER_H
#include <stdint.h>

#include "generic-layer.h"
#include "misc.h"

typedef struct{
    uint8_t level;

    uint32_t left;
    uint32_t right;
    uint32_t top;
    uint32_t bottom;
}MapTileProviderArea;

typedef struct{
    /* In TMS mode, the Y axis (tiles coordinates within the world map)
     * is reversed*/
    bool is_tms;
    char *base;

    char *lvl;
    char *tilex;
    char *tiley;
}MapProviderUrlTemplate;

typedef struct{
    char *home;
    char *format; /*tile file extension*/
    MapProviderUrlTemplate url;

    intf8_t priority;

    MapTileProviderArea *areas;
    size_t nareas;
}MapTileProvider;


MapTileProvider *map_tile_provider_new(const char *home, const char *format, intf8_t priority);

MapTileProvider *map_tile_provider_init(MapTileProvider *self, const char *home,
                                        const char *format, intf8_t priority);

MapTileProvider *map_tile_provider_dispose(MapTileProvider *self);
MapTileProvider *map_tile_provider_free(MapTileProvider *self);

GenericLayer *map_tile_provider_get_tile(MapTileProvider *self, uintf8_t level, uint32_t x, uint32_t y);

int map_tile_provider_compare(MapTileProvider *self, MapTileProvider *other);
int map_tile_provider_compare_ptr(MapTileProvider **self, MapTileProvider **other);
#endif /* MAP_TILE_PROVIDER_H */

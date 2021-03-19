#ifndef MAP_TILE_PROVIDER_H
#define MAP_TILE_PROVIDER_H
#include <stdint.h>

#include "generic-layer.h"
#include "misc.h"

typedef struct{
    uintf16_t x;
    uintf16_t y;
    uintf8_t level;
    GenericLayer *layer;
}MapTileDescriptor;

typedef struct{
    uint8_t level;

    uint32_t left;
    uint32_t right;
    uint32_t top;
    uint32_t bottom;
}MapTileProviderArea;

typedef struct{
    char *base;

    char *lvl;
    char *tilex;
    char *tiley;
}MapProviderUrlTemplate;

typedef struct{
    MapTileDescriptor *tile_cache;
    size_t acache; /*allocated size*/
    size_t ncached; /*currently holding*/

    char *home;
    char *format; /*tile file extension*/
    MapProviderUrlTemplate url;

    MapTileProviderArea *areas;
    size_t nareas;
}MapTileProvider;

#define map_tile_descriptor_match(self, level, x, y) ((self)->level == level && (self)->x == x && (self)->y == y)

MapTileProvider *map_tile_provider_new(const char *home, const char *format,
                                       uintf8_t cache_size);
MapTileProvider *map_tile_provider_init(MapTileProvider *self, const char *home,
                                        const char *format, uintf8_t cache_size);

MapTileProvider *map_tile_provider_dispose(MapTileProvider *self);
MapTileProvider *map_tile_provider_free(MapTileProvider *self);

bool map_tile_provider_set_cache_size(MapTileProvider *self, uintf8_t cache_size);
GenericLayer *map_tile_provider_get_tile(MapTileProvider *self, uintf8_t level, uint32_t x, uint32_t y);

#endif /* MAP_TILE_PROVIDER_H */

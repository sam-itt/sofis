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
    MapTileDescriptor *tile_cache;
    size_t acache; /*allocated size*/
    size_t ncached; /*currently holding*/
}MapTileProvider;

#define map_tile_descriptor_match(self, level, x, y) ((self)->level == level && (self)->x == x && (self)->y == y)

MapTileProvider *map_tile_provider_new(uintf8_t cache_size);
MapTileProvider *map_tile_provider_init(MapTileProvider *self, uintf8_t cache_size);

MapTileProvider *map_tile_provider_dispose(MapTileProvider *self);
MapTileProvider *map_tile_provider_free(MapTileProvider *self);

bool map_tile_provider_set_cache_size(MapTileProvider *self, uintf8_t cache_size);
GenericLayer *map_tile_provider_get_tile(MapTileProvider *self, uintf8_t level, uint32_t x, uint32_t y);

#endif /* MAP_TILE_PROVIDER_H */
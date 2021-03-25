#ifndef MAP_TILE_CACHE_H
#define MAP_TILE_CACHE_H
#include <stdio.h>
#include <stdbool.h>

#include "generic-layer.h"
#include "misc.h"

typedef struct{
    Uint32 atime; /*last access time in SDL_Ticks*/

    uintf16_t x;
    uintf16_t y;
    uintf8_t level;
    GenericLayer *layer;
}MapTileDescriptor;

typedef struct{
    MapTileDescriptor *tile_cache;
    size_t acache; /*allocated size*/
    size_t ncached; /*currently holding*/
}MapTileCache;

MapTileCache *map_tile_cache_init(MapTileCache *self, size_t cache_size);
MapTileCache *map_tile_cache_dispose(MapTileCache *self);

bool map_tile_cache_set_size(MapTileCache *self, uintf8_t cache_size);
GenericLayer *map_tile_cache_get(MapTileCache *self,
                                 uintf8_t level, uint32_t x, uint32_t y);
bool map_tile_cache_add(MapTileCache *self, GenericLayer *tile,
                        uintf8_t level, uint32_t x, uint32_t y);


static inline bool map_tile_descriptor_match(MapTileDescriptor *self,
                                             uintf8_t level,
                                             uint32_t x, uint32_t y)
{
    return self->level == level
           && self->x == x
           && self->y == y;
}
#endif /* MAP_TILE_CACHE_H */
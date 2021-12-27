/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>

#include "map-tile-cache.h"
#include "generic-layer.h"
#include "SDL_timer.h"

static MapTileDescriptor *map_tile_cache_oldest(MapTileCache *self);

MapTileCache *map_tile_cache_init(MapTileCache *self, size_t cache_size)
{
    self->acache = cache_size;
    self->tile_cache = calloc(self->acache, sizeof(MapTileDescriptor));
    if(!self->tile_cache)
        return NULL;

    return self;
}


MapTileCache *map_tile_cache_dispose(MapTileCache *self)
{

    for(int i = 0; i < self->ncached; i++)
        generic_layer_unref(self->tile_cache[i].layer);

    if(self->tile_cache)
        free(self->tile_cache);

   return self;
}

bool map_tile_cache_set_size(MapTileCache *self, uintf8_t cache_size)
{
    size_t old_size;
    void *tmp;

    old_size = self->acache;
    self->acache = cache_size;
    tmp = realloc(self->tile_cache, self->acache * sizeof(MapTileDescriptor));
    if(!tmp){
        self->acache= old_size;
        return false;
    }
    self->tile_cache = tmp;
    /*clear cache ?*/
    return true;
}

void map_tile_cache_clear(MapTileCache *self)
{
    for(int i = 0; i < self->ncached; i++)
        generic_layer_unref(self->tile_cache[i].layer);

    self->ncached = 0;
}

GenericLayer *map_tile_cache_get(MapTileCache *self,
                                 uintf8_t level, int32_t x, int32_t y)
{
    for(int i = 0; i < self->ncached; i++){
        if(map_tile_descriptor_match(&self->tile_cache[i],level, x, y)){
            self->tile_cache[i].atime = SDL_GetTicks();
            return self->tile_cache[i].layer;
        }
    }
    return NULL;
}

bool map_tile_cache_add(MapTileCache *self, GenericLayer *tile,
                        uintf8_t level, int32_t x, int32_t y)
{
    generic_layer_ref(tile);
    if(self->ncached == self->acache){
        MapTileDescriptor *slot;

        slot = map_tile_cache_oldest(self);
        generic_layer_unref(slot->layer);

        *slot = (MapTileDescriptor){
            .layer = tile,
            .level = level,
            .x = x,
            .y = y
        };
    }else{
        self->tile_cache[self->ncached++] = (MapTileDescriptor){
            .layer = tile,
            .level = level,
            .x = x,
            .y = y
        };
    }

    return true;
}

/**
 * @brief Returns the cache location used by the least used descriptor
 * (least recent last usage)
 *
 * MapTileDescriptor internal usage, not meant to be used by client code
 *
 * @param self a MapTileCache
 * @return The slot location
 */
static MapTileDescriptor *map_tile_cache_oldest(MapTileCache *self)
{
    Uint32 now;
    MapTileDescriptor *rv;
    int rv_idx;

    now = SDL_GetTicks();
    rv_idx = 0;
    for(int i = 0; i < self->ncached; i++){
        if(self->tile_cache[i].atime < now){
            rv_idx = i;
            now = self->tile_cache[i].atime;
        }
    }
    return &self->tile_cache[rv_idx];
}


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include "generic-layer.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "map-tile-provider.h"
#include "misc.h"

#ifndef TILES_ROOT
#define TILES_ROOT "/home/samuel/ecw2/ign-oaci-tiles"
#endif

MapTileProvider *map_tile_provider_new(uintf8_t cache_size)
{
    MapTileProvider *self;

    self = calloc(1, sizeof(MapTileProvider));
    if(self){
        if(!map_tile_provider_init(self, cache_size))
            return map_tile_provider_free(self);
    }
    return self;
}

MapTileProvider *map_tile_provider_init(MapTileProvider *self, uintf8_t cache_size)
{
    self->acache = cache_size;
    self->tile_cache = calloc(self->acache, sizeof(MapTileDescriptor));
    if(!self->tile_cache)
        return NULL;
    return self;
}

MapTileProvider *map_tile_provider_dispose(MapTileProvider *self)
{
    for(int i = 0; i < self->ncached; i++)
        generic_layer_unref(self->tile_cache[i].layer);
    if(self->tile_cache)
        free(self->tile_cache);

    return NULL;
}

MapTileProvider *map_tile_provider_free(MapTileProvider *self)
{
    map_tile_provider_dispose(self);
    free(self);
    return NULL;
}

bool map_tile_provider_set_cache_size(MapTileProvider *self, uintf8_t cache_size)
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

GenericLayer *map_tile_provider_get_tile(MapTileProvider *self, uintf8_t level, uint32_t x, uint32_t y)
{
    char *filename;
    GenericLayer *rv;

    for(int i = 0; i < self->ncached; i++){
        if(map_tile_descriptor_match(&self->tile_cache[i],level, x, y))
            return self->tile_cache[i].layer;
    }

    rv = NULL;
    asprintf(&filename, "%s/%d/%d/%d.jpg", TILES_ROOT, level, x, y);
    if(access(filename, F_OK) != 0) goto out;

    rv = generic_layer_new_from_file(filename);
    if(!rv) goto out;
    generic_layer_build_texture(rv);
    generic_layer_ref(rv);
    if(self->ncached == self->acache){
        //Move all elements down, TODO: do better than that.
        /*last element will be overriden*/
        generic_layer_unref(self->tile_cache[self->ncached-1].layer);
        for(int i = self->ncached-1; i > 0; i--){
            self->tile_cache[i] = self->tile_cache[i-1];
        }
        self->tile_cache[0] = (MapTileDescriptor){
            .layer = rv,
            .level = level,
            .x = x,
            .y = y
        };
    }else{
        self->tile_cache[self->ncached++] = (MapTileDescriptor){
            .layer = rv,
            .level = level,
            .x = x,
            .y = y
        };
    }
out:
    free(filename);
    return rv;
}

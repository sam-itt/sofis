/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "map-tile-provider.h"
#include "misc.h"

MapTileProvider *map_tile_provider_init(MapTileProvider *self,
                                        MapTileProviderOps *ops, intf8_t priority)
{
    self->ops = ops;
    self->priority = priority;
    return self;
}

MapTileProvider *map_tile_provider_dispose(MapTileProvider *self)
{
    if(self->areas)
        free(self->areas);
    return self;
}

MapTileProvider *map_tile_provider_free(MapTileProvider *self)
{
    if(self->ops->dispose)
        self->ops->dispose(MAP_TILE_PROVIDER(self));
    map_tile_provider_dispose(self);
    free(self);
    return NULL;
}

bool map_tile_provider_set_nareas(MapTileProvider *self, size_t nareas)
{
    if(nareas > self->nareas){
        void *tmp = realloc(self->areas, sizeof(MapTileProviderArea)*nareas);
        if(!tmp)
            return false;
        memset(
            (MapTileProviderArea*)tmp + self->nareas,
            0,
            (nareas - self->nareas) * sizeof(MapTileProviderArea)
        );
        self->areas = tmp;
        self->nareas = nareas;
    }
    return true;
}

bool map_tile_provider_has_tile(MapTileProvider *self, uintf8_t level, int32_t x, int32_t y)
{
    int i;

    for(i = 0; i < self->nareas; i++){
        if(self->areas[i].level == level){
            return(   (x >= self->areas[i].left) && (x <= self->areas[i].right)
                   && (y >= self->areas[i].top)  && (y <= self->areas[i].bottom)
            );
        }
    }

    /*true if no area is registered, false if there is any and we got here*/
    return (i == 0);
}

int map_tile_provider_compare(MapTileProvider *self, MapTileProvider *other)
{
    if(self->priority < other->priority)
        return -1;
    if(self->priority > other->priority)
        return 1;
    return 0;
}

int map_tile_provider_compare_ptr(MapTileProvider **self, MapTileProvider **other)
{
    return map_tile_provider_compare(*self, *other);
}


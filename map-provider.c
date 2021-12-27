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

#include "map-provider.h"
#include "misc.h"

MapProvider *map_provider_init(MapProvider *self,
                               MapProviderOps *ops, intf8_t priority)
{
    self->ops = ops;
    self->priority = priority;
    return self;
}

MapProvider *map_provider_dispose(MapProvider *self)
{
    if(self->areas)
        free(self->areas);

    if(self->ops->dispose)
        self->ops->dispose(MAP_PROVIDER(self));
    return self;
}

MapProvider *map_provider_free(MapProvider *self)
{
    free(map_provider_dispose(self));
    return NULL;
}

bool map_provider_set_nareas(MapProvider *self, size_t nareas)
{
    if(nareas > self->nareas){
        void *tmp = realloc(self->areas, sizeof(MapProviderArea)*nareas);
        if(!tmp)
            return false;
        memset(
            (MapProviderArea*)tmp + self->nareas,
            0,
            (nareas - self->nareas) * sizeof(MapProviderArea)
        );
        self->areas = tmp;
        self->nareas = nareas;
    }
    return true;
}

bool map_provider_has_tile(MapProvider *self, uintf8_t level, int32_t x, int32_t y)
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

int map_provider_compare(MapProvider *self, MapProvider *other)
{
    if(self->priority < other->priority)
        return -1;
    if(self->priority > other->priority)
        return 1;
    return 0;
}

int map_provider_compare_ptr(MapProvider **self, MapProvider **other)
{
    return map_provider_compare(*self, *other);
}


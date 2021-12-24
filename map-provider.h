/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MAP_PROVIDER_H
#define MAP_PROVIDER_H
#include <stdint.h>

#include "generic-layer.h"
#include "misc.h"

/* MAP_GAUGE_MAX_LEVEL 23
 * has 8388608 tiles from 0 to 8388607
 * which needs 24 bits. Nearest type is int32
 */
typedef struct{
    uint8_t level;

    int32_t left;
    int32_t right;
    int32_t top;
    int32_t bottom;
}MapProviderArea;

typedef struct _MapProvider MapProvider;
typedef GenericLayer *(*MapProviderGetTileFunc)(MapProvider *self,
                                                   uintf8_t level,
                                                   int32_t x, int32_t y);
typedef MapProvider *(*MapProviderDisposeFunc)(MapProvider *self);
typedef struct{
    MapProviderGetTileFunc get_tile;
    MapProviderDisposeFunc dispose;
}MapProviderOps;

typedef struct _MapProvider{
    MapProviderOps *ops;
    intf8_t priority;

    MapProviderArea *areas;
    size_t nareas;
}MapProvider;

#define MAP_PROVIDER(self) ((MapProvider*)(self))

MapProvider *map_provider_init(MapProvider *self,
                                MapProviderOps *ops, intf8_t priority);
MapProvider *map_provider_dispose(MapProvider *self);
MapProvider *map_provider_free(MapProvider *self);

bool map_provider_set_nareas(MapProvider *self, size_t nareas);

bool map_provider_has_tile(MapProvider *self, uintf8_t level, int32_t x, int32_t y);
static inline GenericLayer *map_provider_get_tile(MapProvider *self, uintf8_t level, int32_t x, int32_t y)
{
    return self->ops->get_tile(self, level, x, y);
}


int map_provider_compare(MapProvider *self, MapProvider *other);
int map_provider_compare_ptr(MapProvider **self, MapProvider **other);
#endif /* MAP_PROVIDER_H */

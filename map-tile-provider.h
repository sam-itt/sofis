/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MAP_TILE_PROVIDER_H
#define MAP_TILE_PROVIDER_H
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
}MapTileProviderArea;

typedef struct _MapTileProvider MapTileProvider;
typedef GenericLayer *(*MapTileProviderGetTileFunc)(MapTileProvider *self,
                                                   uintf8_t level,
                                                   int32_t x, int32_t y);
typedef MapTileProvider *(*MapTileProviderDisposeFunc)(MapTileProvider *self);
typedef struct{
    MapTileProviderGetTileFunc get_tile;
    MapTileProviderDisposeFunc dispose;
}MapTileProviderOps;

typedef struct _MapTileProvider{
    MapTileProviderOps *ops;
    intf8_t priority;

    MapTileProviderArea *areas;
    size_t nareas;
}MapTileProvider;

#define MAP_TILE_PROVIDER(self) ((MapTileProvider*)(self))

MapTileProvider *map_tile_provider_init(MapTileProvider *self,
                                        MapTileProviderOps *ops, intf8_t priority);
MapTileProvider *map_tile_provider_dispose(MapTileProvider *self);
MapTileProvider *map_tile_provider_free(MapTileProvider *self);

bool map_tile_provider_set_nareas(MapTileProvider *self, size_t nareas);

bool map_tile_provider_has_tile(MapTileProvider *self, uintf8_t level, int32_t x, int32_t y);
static inline GenericLayer *map_tile_provider_get_tile(MapTileProvider *self, uintf8_t level, int32_t x, int32_t y)
{
    return self->ops->get_tile(self, level, x, y);
}


int map_tile_provider_compare(MapTileProvider *self, MapTileProvider *other);
int map_tile_provider_compare_ptr(MapTileProvider **self, MapTileProvider **other);
#endif /* MAP_TILE_PROVIDER_H */

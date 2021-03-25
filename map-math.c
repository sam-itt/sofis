/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "map-math.h"
#include "misc.h"

#define MIN_LATITUDE  -85.05112878
#define MAX_LATITUDE   85.05112878
#define MIN_LONGITUDE -180
#define MAX_LONGITUDE  180


bool map_math_geo_to_pixel(double latitude, double longitude, uintf8_t level, uint32_t *px, uint32_t *py)
{
    latitude = map_math_clip(latitude, MIN_LATITUDE, MAX_LATITUDE);
    longitude = map_math_clip(longitude, MIN_LONGITUDE, MAX_LONGITUDE);

    double x = (longitude + 180) / 360;
    double sinLatitude = sin(latitude * M_PI / 180);
    double y = 0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * M_PI);

    uint mapSize = map_math_size(level);
    *px = (uint32_t) map_math_clip(x * mapSize + 0.5, 0, mapSize - 1);
    *py = (uint32_t) map_math_clip(y * mapSize + 0.5, 0, mapSize - 1);
    return true;
}

void map_math_pixel_to_geo(uint32_t px, uint32_t py, uintf8_t level, double *latitude, double *longitude)
{
    double mapSize = map_math_size(level);
    double x = (map_math_clip(px, 0, mapSize - 1) / mapSize) - 0.5;
    double y = 0.5 - (map_math_clip(py, 0, mapSize - 1) / mapSize);

    *latitude = 90 - 360 * atan(exp(-y * 2 * M_PI)) / M_PI;
    *longitude = 360 * x;
}

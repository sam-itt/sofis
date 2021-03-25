/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef FISHBONE_GAUGE_H
#define FISHBONE_GAUGE_H

#include "sfv-gauge.h"
#include "generic-layer.h"
#include "generic-ruler.h"

typedef struct{
    /* Ruler offset within the gauge
     * Cursor base being at y = 0
     * Simple ints would have done but the blitting
     * functions need SDL_Rects
     */
    SDL_Rect cursor_rect;
}FishboneGaugeState;

typedef struct{
    SfvGauge super;

    float value;

    GenericRuler ruler;
    Uint32 color;

    /* TODO: Generate cursors otf, and support
     * 2 cursors, one of each side of the scale.
     * */
    GenericLayer *cursor;

    ColorZone *zones;
    uint8_t nzones;

    /* Ruler offset within the gauge
     * Cursor base being at y = 0
     * Simple ints would have done but the blitting
     * functions need SDL_Rects
     */
    SDL_Rect ruler_rect;

    FishboneGaugeState state;
}FishboneGauge;

FishboneGauge *fishbone_gauge_new(bool marked,
                                  PCF_Font *font, SDL_Color color,
                                  float from, float to, float step,
                                  int bar_max_w, int bar_max_h,
                                  int nzones, ColorZone *zones);
FishboneGauge *fishbone_gauge_init(FishboneGauge *self,
                                   bool marked,
                                   PCF_Font *font, SDL_Color color,
                                   float from, float to, float step,
                                   int bar_max_w, int bar_max_h,
                                   int nzone, ColorZone *zones);

bool fishbone_gauge_set_value(FishboneGauge *self, float value, bool animated);
#endif /* FISHBONE_GAUGE_H */

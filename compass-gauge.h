/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef COMPASS_GAUGE_H
#define COMPASS_GAUGE_H

#include "sfv-gauge.h"
#include "generic-layer.h"
#include "text-gauge.h"

typedef struct{
#if !USE_SDL_GPU
    SDL_Surface *rbuffer; /*rotation buffer*/
#endif
}CompassGaugeState;

typedef struct{
	SfvGauge super;

    GenericLayer outer;
    GenericLayer inner;

    TextGauge *caption;

    SDL_Point icenter;
    SDL_Rect inner_rect;
    SDL_Rect outer_rect;
#if !USE_SDL_GPU
    SDL_Renderer *renderer;
    SDL_Texture *texture;
#endif
    CompassGaugeState state;
}CompassGauge;


CompassGauge *compass_gauge_new(void);
CompassGauge *compass_gauge_init(CompassGauge *self);

bool compass_gauge_set_value(CompassGauge *self, float value, bool animated);
#endif /* COMPASS_GAUGE_H */

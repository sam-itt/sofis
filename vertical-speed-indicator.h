/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef VERTICAL_SPEED_INDICATOR_H
#define VERTICAL_SPEED_INDICATOR_H
#include "SDL_pcf.h"

#include "resource-manager.h"
#include "sfv-gauge.h"
#include "generic-layer.h"
#include "generic-ruler.h"

#define VS_VALUE_MAX_LEN 6 /*5 digits plus \0*/

typedef struct{
    SDL_Rect cloc; /*cursor location*/
    SDL_Rect tloc; /*text location*/

    /* -1 because we don't need to render the \0*/
    PCF_StaticFontPatch chars[VS_VALUE_MAX_LEN-1];
    int nchars;
}VerticalSpeedIndicatorState;

typedef struct{
    SfvGauge super;

    GenericLayer cursor; /*cursor background image*/

    PCF_StaticFont *font;

    GenericRuler scale;

    VerticalSpeedIndicatorState state;
}VerticalSpeedIndicator;

VerticalSpeedIndicator *vertical_speed_indicator_new(int width, int height, FontResource font);
VerticalSpeedIndicator *vertical_speed_indicator_init(VerticalSpeedIndicator *self, int width, int height, FontResource font);

bool vertical_speed_indicator_set_value(VerticalSpeedIndicator *self, float value, bool animated);
#endif /* VERTICAL_SPEED_INDICATOR_H */

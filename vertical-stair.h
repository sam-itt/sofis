/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef VERTICAL_STAIR_H
#define VERTICAL_STAIR_H
#include "SDL_pcf.h"

#include "resource-manager.h"
#include "sfv-gauge.h"
#include "generic-layer.h"
#include "vertical-strip.h"

#define VS_VALUE_MAX_LEN 6 /*5 digits plus \0*/

typedef struct{
    SDL_Rect cloc; /*cursor location*/
    SDL_Rect tloc; /*text location*/

    /* -1 because we don't need to render the \0*/
    PCF_StaticFontPatch chars[VS_VALUE_MAX_LEN-1];
    int nchars;
}VerticalStairState;

typedef struct{
    SfvGauge super;

    GenericLayer cursor; /*cursor background image*/

    PCF_StaticFont *font;

    VerticalStrip scale;

    VerticalStairState state;
}VerticalStair;

VerticalStair *vertical_stair_new(int width, int height, FontResource font);
VerticalStair *vertical_stair_init(VerticalStair *self, int width, int height, FontResource font);

bool vertical_stair_set_value(VerticalStair *self, float value, bool animated);
#endif /* VERTICAL_STAIR_H */

/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef GENERIC_RULER_H
#define GENERIC_RULER_H

#include "generic-layer.h"

#include "SDL_pcf.h"

typedef enum __attribute__ ((__packed__)){
    RulerVertical,
    RulerHorizontal,
    NRulerOrientations
}RulerOrientation;

typedef enum __attribute__ ((__packed__)){
    RulerGrowAlongAxis = 0,    /*Grows left to right, top to bottom*/
    RulerGrowAgainstAxis = 1,  /*Values grow in reverse with respect to the axis*/
}RulerDirection;

typedef enum __attribute__ ((__packed__)){
    LocationNone = 0,
    Top,
    Center,
    Bottom,
    Left,
    Right,
    NLocations
}Location;

/*TODO: Use only 2 bits*/
typedef enum __attribute__ ((__packed__)){
    FromIncluded = 1 << 0, /*00000001*/
    FromExcluded = 1 << 1, /*00000010*/
    ToIncluded =   1 << 2, /*00000100*/
    ToExcluded =   1 << 3  /*00001000*/
}BoundFlags;

typedef struct{
    float from;
    float to;
    BoundFlags flags;
    SDL_Color color;
}ColorZone;


/**
 * (Really) Generic mapping between a range of values {@value #start},  {@value #start}
 * and an image.
 */
typedef struct{
    GenericLayer super;

    RulerOrientation orientation;
    RulerDirection direction;
    SDL_Rect ruler_area; /*ruler area*/
    float start; /*first (included) value in the range*/
    float end;  /*last (included) value in the range*/
    float hatch_step; /*Spacing between hatch marks. TODO: arbitrary number of hatch levels/levels*/

    float ppv; /*scale, in pixels per value*/
}GenericRuler;

GenericRuler *generic_ruler_init(GenericRuler *self,
                                 RulerOrientation orientation,
                                 RulerDirection direction,
                                 float start, float end, float step,
                                 PCF_Font *markings_font,
                                 Location markings_location,
                                 int8_t markings_precision,
                                 int ruler_w, int ruler_h);

void generic_ruler_dispose(GenericRuler *self);

bool generic_ruler_draw_zones(GenericRuler *self, Location spine_location, int nzones, ColorZone *zones, float fill_ratio);
bool generic_ruler_etch_hatches(GenericRuler *self, Uint32 color, bool etch_spine, bool etch_hatches, Location spine_location);
bool generic_ruler_etch_markings(GenericRuler *self, Location markings_location, PCF_Font *font, Uint32 color, int8_t precision);

int generic_ruler_get_pixel_increment_for(GenericRuler *self, float value);
void generic_ruler_clip_value(GenericRuler *self, float *value);
#endif /* GENERIC_RULER_H */

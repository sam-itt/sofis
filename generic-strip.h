/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef GENERIC_STRIP_H
#define GENERIC_STRIP_H

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL_gpu.h>

#include "SDL_render.h"
#include "generic-layer.h"

typedef enum __attribute__ ((__packed__)){
    ORIENTATION_VERTICAL,
    ORITENTATION_HORIZONTAL,
}StripOrientation;

/**
 * Generic mapping between a range of values {@value #start},  {@value #start}
 * and an image.
 *
 */
typedef struct{
    GenericLayer super;

    StripOrientation orientation;
    float ppv; /* pixels per value*/
    float start, end; /*Range*/
}GenericStrip;

#define GENERIC_STRIP(self) ((GenericStrip *)(self))

void generic_strip_dispose(GenericStrip *self);

bool generic_strip_has_value(GenericStrip *self, float value);
float generic_strip_resolve_value(GenericStrip *self, float value, bool reverse);
void generic_strip_clip_value(GenericStrip *self, float *value);

#endif /* GENERIC_STRIP_H */

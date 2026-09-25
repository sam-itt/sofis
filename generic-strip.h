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

#include "generic-layer.h"

typedef enum __attribute__ ((__packed__)){
    ORIENTATION_VERTICAL = 0 << 0,
    ORIENTATION_HORIZONTAL = 1 << 0,

    GROW_ALONG_AXIS = 0 << 1,    /*Values grows left to right, top to bottom*/
    GROW_AGAINST_AXIS = 1 << 1,
}StripFlags;

#define ORIENTATION_MASK (1 << 0)
#define DIRECTION_MASK   (1 << 1)

/**
 * Generic mapping between a range of values {@value #start},  {@value #start}
 * and an image.
 *
 */
typedef struct{
    GenericLayer super;

    StripFlags flags;
    float ppv; /* pixels per value*/
    float start, end; /*Range*/
}GenericStrip;

#define GENERIC_STRIP(self) ((GenericStrip *)(self))

void generic_strip_dispose(GenericStrip *self);

bool generic_strip_has_value(GenericStrip *self, float value);
float generic_strip_resolve_value(GenericStrip *self, float value);
void generic_strip_clip_value(GenericStrip *self, float *value);

#endif /* GENERIC_STRIP_H */

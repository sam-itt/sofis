/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef GENERIC_LAYER_H
#define GENERIC_LAYER_H
#include "misc.h"
#include <stdbool.h>

#include <SDL2/SDL.h>

#if USE_SDL_GPU
#include <SDL_gpu.h>
#endif

typedef struct{
    uintf8_t refcount;
    SDL_Surface *canvas;
#if USE_SDL_GPU
    GPU_Image *texture;
#endif
}GenericLayer;

#define GENERIC_LAYER(self) ((GenericLayer *)(self))
#define generic_layer_lock(self) SDL_LockSurface((self)->canvas)
#define generic_layer_unlock(self) SDL_UnlockSurface((self)->canvas)

#define generic_layer_w(self) ((self)->canvas->w)
#define generic_layer_h(self) ((self)->canvas->h)

GenericLayer *generic_layer_new(int width, int height);
GenericLayer *generic_layer_new_from_file(const char *filename);

bool generic_layer_init(GenericLayer *self, int width, int height);
bool generic_layer_init_with_masks(GenericLayer *self, int width, int height, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
bool generic_layer_init_from_file(GenericLayer *self, const char *filename);

void generic_layer_dispose(GenericLayer *self);
void generic_layer_free(GenericLayer *self);

void generic_layer_ref(GenericLayer *self);
void generic_layer_unref(GenericLayer *self);

bool generic_layer_build_texture(GenericLayer *self);
void generic_layer_update_texture(GenericLayer *self);
#endif /* GENERIC_LAYER_H */

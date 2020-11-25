#ifndef VERTICAL_STRIP_H
#define VERTICAL_STRIP_H

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL_gpu.h>

#include "SDL_render.h"
#include "generic-layer.h"
/**
 * Generic mapping between a range of values {@value #start},  {@value #start}
 * and an image.
 *
 * TODO: Replace with GenericRuler
 */
typedef struct{
    GenericLayer super;

    float ppv; /* pixels per value*/
    float start, end; /*Range*/
}VerticalStrip;

#define VERTICAL_STRIP(self) ((VerticalStrip *)(self))

void vertical_strip_dispose(VerticalStrip *self);

bool vertical_strip_has_value(VerticalStrip *self, float value);
float vertical_strip_resolve_value(VerticalStrip *self, float value, bool reverse);
void vertical_strip_clip_value(VerticalStrip *self, float *value);

#endif /* VERTICAL_STRIP_H */

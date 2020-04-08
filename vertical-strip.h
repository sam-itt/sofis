#ifndef VERTICAL_STRIP_H
#define VERTICAL_STRIP_H

#include <stdbool.h>

#include <SDL2/SDL.h>

typedef struct{
    SDL_Surface *ruler;

    float fvo; /*First value offset*/
    float ppv; /* pixels per value*/
    float start, end; /*Range*/
    float vstep; /* increment between two major graduations*/
}VerticalStrip;

#define VERTICAL_STRIP(self) ((VerticalStrip *)(self))

void vertical_strip_dispose(VerticalStrip *self);

bool vertical_strip_has_value(VerticalStrip *self, float value);
float vertical_strip_resolve_value(VerticalStrip *self, float value, bool reverse);
void vertical_strip_clip_value(VerticalStrip *self, float *value);
#endif /* VERTICAL_STRIP_H */

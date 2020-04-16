#ifndef MISC_H
#define MISC_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "SDL_rect.h"

typedef uint_fast8_t uintf8_t;
typedef uint_fast16_t uintf16_t;

int number_split(float n, float *parts, size_t p_size);
int number_digits(float n);

bool interval_intersect(float as, float ae, float bs, float be, float *is, float *ie);
void SDLExt_RectCenter(SDL_Rect *self, SDL_Rect *reference);
void SDLExt_RectDump(SDL_Rect *self);
#endif /* MISC_H */

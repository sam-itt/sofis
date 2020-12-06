#ifndef DIGIT_BARREL_H
#define DIGIT_BARREL_H

#include <SDL2/SDL.h>

#include "generic-layer.h"
#include "vertical-strip.h"
#include "misc.h"
#include "buffered-gauge.h"

typedef struct{
    GenericLayer *top;
    SDL_Rect top_rect;
    SDL_Rect top_rect_to;

    GenericLayer *middle;
    SDL_Rect middle_rect;
    SDL_Rect middle_rect_to;

    GenericLayer *bottom;
    SDL_Rect bottom_rect;
    SDL_Rect bottom_rect_to;
}DigitBarrelState;

typedef struct{
    VerticalStrip super;

    float symbol_h;
    float fei;
    uintf8_t refcount;
}DigitBarrel;


DigitBarrel *digit_barrel_new(PCF_Font *font, float start, float end, float step);
DigitBarrel *digit_barrel_init(DigitBarrel *self, PCF_Font *font, float start, float end, float step);
void digit_barrel_free(DigitBarrel *self);


void digit_barrel_draw_etch_marks(DigitBarrel *self);
float digit_barrel_resolve_value(DigitBarrel *self, float value);

//void digit_barrel_render_value(DigitBarrel *self, float value, BufferedGauge *dst, SDL_Rect *region, float rubis);
void digit_barrel_state_value(DigitBarrel *self, float value, SDL_Rect *region, float rubis, DigitBarrelState *state);
#endif /* DIGIT_BARREL_H */

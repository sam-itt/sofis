#ifndef DIGIT_BARREL_H
#define DIGIT_BARREL_H

#include <SDL2/SDL.h>

#include "vertical-strip.h"
#include "misc.h"
#include "buffered-gauge.h"

typedef struct{
    VerticalStrip super;

    float symbol_h;
    float fei;
    uintf8_t refcount;
}DigitBarrel;


DigitBarrel *digit_barrel_new(uintf8_t font_size, float start, float end, float step);
DigitBarrel *digit_barrel_init(DigitBarrel *self, uintf8_t font_size, float start, float end, float step);
void digit_barrel_free(DigitBarrel *self);


void digit_barrel_draw_etch_marks(DigitBarrel *self);
float digit_barrel_resolve_value(DigitBarrel *self, float value);

void digit_barrel_render_value(DigitBarrel *self, float value, BufferedGauge *dst, SDL_Rect *region, float rubis);
#endif /* DIGIT_BARREL_H */

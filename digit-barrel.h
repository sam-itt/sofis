#ifndef DIGIT_BARREL_H
#define DIGIT_BARREL_H

#include <SDL2/SDL.h>

#include "generic-layer.h"
#include "vertical-strip.h"
#include "misc.h"
#include "buffered-gauge.h"

typedef struct{
    SDL_Rect src;
    SDL_Rect dst;
}DigitBarrelPatch;

typedef struct{
    GenericLayer *layer; /*All patches have the same source*/
    /*TODO: Check if we can get away with 2: Is there a case
     * when all 3 are needed?*/
    DigitBarrelPatch patches[3]; /*Up to 3: top, middle, bottom*/
    uintf8_t npatches;
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

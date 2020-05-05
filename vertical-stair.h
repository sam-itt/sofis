#ifndef VERTICAL_STAIR_H
#define VERTICAL_STAIR_H
#include "sdl-pcf/SDL_pcf.h"

#include "animated-gauge.h"
#include "vertical-strip.h"


typedef struct{
    AnimatedGauge super;

    SDL_Surface *cursor; /*cursor background image*/
    PCF_Font *font;

    VerticalStrip scale;
}VerticalStair;

VerticalStair *vertical_stair_new(const char *bg_img, const char *cursor_img, PCF_Font *font);
VerticalStair *vertical_stair_init(VerticalStair *self, const char *bg_img, const char *cursor_img, PCF_Font *font);
void vertical_stair_dispose(VerticalStair *self);
void vertical_stair_free(VerticalStair *self);

#endif /* VERTICAL_STAIR_H */

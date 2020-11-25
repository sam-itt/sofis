#ifndef VERTICAL_STAIR_H
#define VERTICAL_STAIR_H
#include "SDL_render.h"
#include "SDL_pcf.h"

#include "animated-gauge.h"
#include "generic-layer.h"
#include "vertical-strip.h"


typedef struct{
    AnimatedGauge super;

    GenericLayer cursor; /*cursor background image*/

    PCF_StaticFont *font;

    VerticalStrip scale;
}VerticalStair;

VerticalStair *vertical_stair_new(const char *bg_img, const char *cursor_img, PCF_StaticFont *font);
VerticalStair *vertical_stair_init(VerticalStair *self, const char *bg_img, const char *cursor_img, PCF_StaticFont *font);
void vertical_stair_dispose(VerticalStair *self);
void vertical_stair_free(VerticalStair *self);

#endif /* VERTICAL_STAIR_H */

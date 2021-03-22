#ifndef VERTICAL_STAIR_H
#define VERTICAL_STAIR_H
#include "SDL_render.h"
#include "SDL_pcf.h"

#include "sfv-gauge.h"
#include "generic-layer.h"
#include "vertical-strip.h"

#define VS_VALUE_MAX_LEN 6 /*5 digits plus \0*/

typedef struct{
    SDL_Rect cloc; /*cursor location*/
    SDL_Rect tloc; /*text location*/

    /* -1 because we don't need to render the \0*/
    PCF_StaticFontPatch chars[VS_VALUE_MAX_LEN-1];
    int nchars;
}VerticalStairState;

typedef struct{
    SfvGauge super;

    GenericLayer cursor; /*cursor background image*/

    PCF_StaticFont *font;

    VerticalStrip scale;

    VerticalStairState state;
}VerticalStair;

VerticalStair *vertical_stair_new(const char *bg_img, const char *cursor_img, PCF_StaticFont *font);
VerticalStair *vertical_stair_init(VerticalStair *self, const char *bg_img, const char *cursor_img, PCF_StaticFont *font);

bool vertical_stair_set_value(VerticalStair *self, float value, bool animated);
#endif /* VERTICAL_STAIR_H */

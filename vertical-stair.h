#ifndef VERTICAL_STAIR_H
#define VERTICAL_STAIR_H
#include "sdl-pcf/SDL_pcf.h"

#include "animated-gauge.h"
#include "vertical-strip.h"


typedef struct{
    SDL_Surface *bg;

    PCF_Font *font;
    float value;
}VerticalStairCursor;


typedef struct{
    AnimatedGauge super;

    VerticalStrip scale;
    VerticalStairCursor cursor;
}VerticalStair;

VerticalStairCursor *vertical_stair_cursor_init(VerticalStairCursor *self, const char *filename, PCF_Font *font);
void vertical_stair_cursor_dispose(VerticalStairCursor *self);
void vertical_stair_cursor_set_value(VerticalStairCursor *self, float value);

VerticalStair *vertical_stair_new(const char *bg_img, const char *cursor_img, PCF_Font *font);
VerticalStair *vertical_stair_init(VerticalStair *self, const char *bg_img, const char *cursor_img, PCF_Font *font);
void vertical_stair_free(VerticalStair *self);

#endif /* VERTICAL_STAIR_H */

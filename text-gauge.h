#ifndef TEXT_GAUGE_H
#define TEXT_GAUGE_H
#include <stdbool.h>

#include "base-gauge.h"
#include "SDL_pcf.h"
#include "generic-layer.h"
#include "view.h"

#define TEXT_COLOR 0
#define BACKGROUND_COLOR 1

typedef struct{
     SDL_Point src;
     SDL_Point dst;
}TextGaugeFontPatch;

typedef struct{
    TextGaugeFontPatch *chars;
    size_t achars; /*Allocated chars*/
}TextGaugeState;

typedef struct{
    BaseGauge super;

    PCFWrapFont font;
    SDL_Color text_color;
    SDL_Color bg_color;
    uint8_t alignment;
    bool outlined;

    char *value;
    size_t asize; /*Allocated size*/
    size_t len; /*data size*/

    TextGaugeState state;
    /* Back buffer to use with regular fonts*/
    GenericLayer *buffer;
}TextGauge;

TextGauge *text_gauge_new(const char *value, bool outlined, int w, int h);
TextGauge *text_gauge_init(TextGauge *self, const char *value, bool outlined, int w, int h);
void text_gauge_free(TextGauge *self);

bool text_gauge_set_value(TextGauge *self, const char *value);
void text_gauge_set_color(TextGauge *self, SDL_Color color, Uint8 which);

void text_gauge_set_font(TextGauge *self, PCF_Font *font);
void text_gauge_set_static_font(TextGauge *self, PCF_StaticFont *font);
#endif /* TEXT_GAUGE_H */

#ifndef LADDER_GAUGE_H
#define LADDER_GAUGE_H
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "basic-animation.h"
#include "vertical-strip.h"
#include "misc.h"

#define PAGE_SIZE 700 /*number of values per page*/
#define N_PAGES 4

typedef enum {TOP_DOWN, BOTTUM_UP} ScrollType;

typedef struct{
    VerticalStrip parent;

    ScrollType direction;
}LadderPage;

typedef struct{
    SDL_Surface *gauge;

    BasicAnimation animation;
    bool damaged;

    float value;
    int rubis;

    LadderPage *pages[N_PAGES];
    uintf8_t base;

    ScrollType direction;
}LadderGauge;


LadderPage *ladder_page_init(LadderPage *self);
float ladder_page_resolve_value(LadderPage *self, float value);

LadderGauge *ladder_gauge_new(ScrollType direction,  int rubis);
void ladder_gauge_free(LadderGauge *self);

void ladder_gauge_set_value(LadderGauge *self, float value);
SDL_Surface *ladder_gauge_render(LadderGauge *self, uint32_t dt);

#endif /* LADDER_GAUGE_H */

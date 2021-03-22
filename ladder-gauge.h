#ifndef LADDER_GAUGE_H
#define LADDER_GAUGE_H
#include <stdbool.h>

#include "sfv-gauge.h"
#include "ladder-page.h"
#include "misc.h"

#define N_PAGES 4

typedef struct{
    GenericLayer *layer;
    SDL_Rect src;
    SDL_Rect dst;
}LadderPagePatch;

typedef struct{
    LadderPagePatch patches[3]; /*Up to 3: top, middle, bottom*/
    uintf8_t npatches;

    int pskip;
}LadderGaugeState;


typedef struct{
    SfvGauge super;

    int rubis;

    LadderPage *pages[N_PAGES];
    uintf8_t base;

    LadderPageDescriptor *descriptor;

    LadderGaugeState state;
}LadderGauge;


LadderGauge *ladder_gauge_new(LadderPageDescriptor *descriptor, int rubis);
LadderGauge *ladder_gauge_init(LadderGauge *self, LadderPageDescriptor *descriptor, int rubis);

bool ladder_gauge_set_value(LadderGauge *self, float value, bool animated);
#endif /* LADDER_GAUGE_H */

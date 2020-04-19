#ifndef LADDER_GAUGE_H
#define LADDER_GAUGE_H
#include <stdbool.h>

#include "ladder-page.h"
#include "animated-gauge.h"
#include "misc.h"

#define N_PAGES 4


typedef struct{
    AnimatedGauge parent;

    int rubis;

    LadderPage *pages[N_PAGES];
    uintf8_t base;

    LadderPageDescriptor *descriptor;
}LadderGauge;


LadderGauge *ladder_gauge_new(LadderPageDescriptor *descriptor, int rubis);
LadderGauge *ladder_gauge_init(LadderGauge *self, LadderPageDescriptor *descriptor, int rubis);
void ladder_gauge_free(LadderGauge *self);

#endif /* LADDER_GAUGE_H */

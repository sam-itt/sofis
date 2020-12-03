#ifndef COMPASS_GAUGE_H
#define COMPASS_GAUGE_H

#include "animated-gauge.h"
#include "generic-layer.h"

typedef struct{
	AnimatedGauge super;

    GenericLayer outer;
    GenericLayer inner;
}CompassGauge;


CompassGauge *compass_gauge_new(void);
CompassGauge *compass_gauge_init(CompassGauge *self);

void compass_gauge_dispose(CompassGauge *self);
void compass_gauge_free(CompassGauge *self);


#endif /* COMPASS_GAUGE_H */

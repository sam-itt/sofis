#ifndef RICH_COMPASS_GAUGE_H
#define RICH_COMPASS_GAUGE_H

#include "base-gauge.h"
#include "compass-gauge.h"
#include "text-gauge.h"


typedef struct{
    BaseGauge super;

    CompassGauge *compass;
    TextGauge *caption;

}RichCompassGauge;


RichCompassGauge *rich_compass_gauge_new(void);
RichCompassGauge *rich_compass_gauge_init(RichCompassGauge *self);

void rich_compass_gauge_dispose(RichCompassGauge *self);
void rich_compass_gauge_free(RichCompassGauge *self);
#endif /* RICH_COMPASS_GAUGE_H */

#ifndef COMPASS_GAUGE_H
#define COMPASS_GAUGE_H

#include "base-gauge.h"
#include "generic-layer.h"
#include "text-gauge.h"

typedef struct{
#if !USE_SDL_GPU
    SDL_Surface *rbuffer; /*rotation buffer*/
#endif
}CompassGaugeState;

typedef struct{
	BaseGauge super;

    float value;
    GenericLayer outer;
    GenericLayer inner;

    TextGauge *caption;

    SDL_Point icenter;
    SDL_Rect inner_rect;
    SDL_Rect outer_rect;
#if !USE_SDL_GPU
    SDL_Renderer *renderer;
#endif
    CompassGaugeState state;
}CompassGauge;


CompassGauge *compass_gauge_new(void);
CompassGauge *compass_gauge_init(CompassGauge *self);

void compass_gauge_dispose(CompassGauge *self);
void compass_gauge_free(CompassGauge *self);

bool compass_gauge_set_value(CompassGauge *self, float value, bool animated);
#endif /* COMPASS_GAUGE_H */

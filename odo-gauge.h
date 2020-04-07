#ifndef ODO_GAUGE_H
#define ODO_GAUGE_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "basic-animation.h"
#include "digit-barrel.h"

typedef struct{
    SDL_Surface *gauge;

    DigitBarrel **barrels;
    int *heights;
    uintf8_t nbarrels;

    int rubis;
    float value;
    float max_value;

    bool damaged;
    BasicAnimation animation;
}OdoGauge;

OdoGauge *odo_gauge_new(DigitBarrel *barrel, int height, int rubis);
OdoGauge *odo_gauge_new_multiple(int rubis, int nbarrels, ...);
OdoGauge *odo_gauge_init(OdoGauge *self, int rubis, int nbarrels, ...);
OdoGauge *odo_gauge_vainit(OdoGauge *self, int rubis, int nbarrels, va_list ap);
void odo_gauge_free(OdoGauge *self);

bool odo_gauge_set_value(OdoGauge *self, float value);
SDL_Surface *odo_gauge_render(OdoGauge *self, Uint32 dt);
#endif /* ODO_GAUGE_H */

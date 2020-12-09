#ifndef ODO_GAUGE_H
#define ODO_GAUGE_H

#include <SDL2/SDL.h>
#include <stdbool.h>

#include "sfv-gauge.h"
#include "digit-barrel.h"


typedef struct{
    DigitBarrelState *barrel_states;
    uintf8_t nbarrel_states; /*Must be the same type as OdoGauge::nbarrels*/

    SDL_Rect *fill_rects;
    uintf8_t nfill_rects; /*Must be the same type as OdoGauge::nbarrels*/

    int rubis_y;
    int pskip;
}OdoGaugeState;

typedef struct{
    SfvGauge super;

    DigitBarrel **barrels;
    int *heights;
    uintf8_t nbarrels;

    int rubis;
    float max_value;

    OdoGaugeState state;
}OdoGauge;

OdoGauge *odo_gauge_new(DigitBarrel *barrel, int height, int rubis);
OdoGauge *odo_gauge_new_multiple(int rubis, int nbarrels, ...);
OdoGauge *odo_gauge_init(OdoGauge *self, int rubis, int nbarrels, ...);
OdoGauge *odo_gauge_vainit(OdoGauge *self, int rubis, int nbarrels, va_list ap);
void odo_gauge_free(OdoGauge *self);

bool odo_gauge_set_value(OdoGauge *self, float value, bool animated);
#endif /* ODO_GAUGE_H */

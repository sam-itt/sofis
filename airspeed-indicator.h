#ifndef AIRSPEED_INDICATOR_H
#define AIRSPEED_INDICATOR_H

#include <SDL2/SDL_ttf.h>

#include "ladder-gauge.h"
#include "airspeed-page-descriptor.h"
#include "odo-gauge.h"

#define RHO_0 1.225 /* kg/m3, Sea level ISA */

typedef struct{
    SDL_Surface *view;
    int w,h;

    LadderGauge *ladder;
    OdoGauge *odo;

    TTF_Font *font;
    int tas;
}AirspeedIndicator;


AirspeedIndicator *airspeed_indicator_new(speed_t v_so, speed_t v_s1,speed_t v_fe,speed_t v_no,speed_t v_ne);
AirspeedIndicator *airspeed_indicator_init(AirspeedIndicator *self, speed_t v_so, speed_t v_s1,speed_t v_fe,speed_t v_no,speed_t v_ne);

bool airspeed_indicator_set_value(AirspeedIndicator *self, float value);
SDL_Surface *airspeed_indicator_render(AirspeedIndicator *self, Uint32 dt);
void airspeed_indicator_render_to(AirspeedIndicator *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);


void airspeed_indicator_dispose(AirspeedIndicator *self);
void airspeed_indicator_free(AirspeedIndicator *self);

#endif /* AIRSPEED_INDICATOR_H */

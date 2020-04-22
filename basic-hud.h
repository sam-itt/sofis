#ifndef BASIC_HUD_H
#define BASIC_HUD_H

#include "base-gauge.h"
#include "alt-group.h"
#include "attitude-indicator.h"
#include "airspeed-indicator.h"

typedef enum{
    ALTITUDE,
    VERTICAL_SPEED,
    AIRSPEED,
    PITCH,
    ROLL,
    SLIP,

    HUD_VALUE_MAX
}HudValue;

enum{
    ALT_GROUP,
    SPEED,

    LOC_MAX
};

typedef struct{
    BaseGauge super;

    AltGroup *altgroup;
    AirspeedIndicator *airspeed;
    AttitudeIndicator *attitude;

    SDL_Rect locations[LOC_MAX];
}BasicHud;


BasicHud *basic_hud_new(void);
BasicHud *basic_hud_init(BasicHud *self);
void basic_hud_dispose(BasicHud *self);
void basic_hud_free(BasicHud *self);

float basic_hud_get(BasicHud *self, HudValue hv);
void basic_hud_set(BasicHud *self, int nvalues, ...);
void basic_hud_set_values(BasicHud *self, int nvalues, va_list ap);

#endif /* BASIC_HUD_H */

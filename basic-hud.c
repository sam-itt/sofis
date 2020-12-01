#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "airspeed-indicator.h"
#include "alt-group.h"
#include "animated-gauge.h"
#include "attitude-indicator.h"
#include "base-gauge.h"
#include "basic-hud.h"
#include "misc.h"

static void basic_hud_render(BasicHud *self, Uint32 dt, RenderTarget destination, SDL_Rect *location);
static BaseGaugeOps basic_hud_ops = {
    .render = (RenderFunc)basic_hud_render
};


BasicHud *basic_hud_new(void)
{
    BasicHud *self;

    self = calloc(1, sizeof(BasicHud));
    if(self){
        if(!basic_hud_init(self)){
            free(self);
            return NULL;
        }
    }
    return self;
}


BasicHud *basic_hud_init(BasicHud *self)
{
    self->altgroup = alt_group_new();
    self->altgroup->altimeter->src = ALT_SRC_GPS;

    self->airspeed = airspeed_indicator_new(50,60,85,155,200);
    self->attitude = attitude_indicator_new(640, 480);

    self->locations[ALT_GROUP] = (SDL_Rect){460,53,0,0};
    self->locations[SPEED] = (SDL_Rect){96,72,0,0};

    base_gauge_init(
        BASE_GAUGE(self),
        &basic_hud_ops,
        640, /*Fixed for now, should be made dynamic and set through params*/
        480
    );

    return self;
}

void basic_hud_dispose(BasicHud *self)
{
    alt_group_free(self->altgroup);
    airspeed_indicator_free(self->airspeed);
    attitude_indicator_free(self->attitude);
}

void basic_hud_free(BasicHud *self)
{
    basic_hud_dispose(self);
    free(self);
}

void basic_hud_set(BasicHud *self, int nvalues, ...)
{
    va_list args;

    va_start(args, nvalues);
    basic_hud_set_values(self, nvalues, args);
    va_end(args);
}

void basic_hud_set_values(BasicHud *self, int nvalues, va_list ap)
{
    HudValue hv;
    double val;

    for(int i = 0; i < nvalues; i++){
        hv = va_arg(ap, HudValue);
        val = va_arg(ap, double);

        switch(hv){
          case ALTITUDE:
            alt_group_set_altitude(self->altgroup, val);
            break;
          case VERTICAL_SPEED:
            alt_group_set_vertical_speed(self->altgroup, val);
            break;
          case AIRSPEED:
            airspeed_indicator_set_value(self->airspeed, val);
            break;
          case PITCH:
            animated_gauge_set_value(ANIMATED_GAUGE(self->attitude), val);
            break;
          case ROLL:
            attitude_indicator_set_roll(self->attitude, -1.0*val);
            break;
        }
    }
}

float basic_hud_get(BasicHud *self, HudValue hv)
{

    switch(hv){
      case ALTITUDE:
        return self->altgroup->altimeter->ladder->super.value;
        break;
      case VERTICAL_SPEED:
        return self->altgroup->vsi->super.value;
        break;
      case AIRSPEED:
        return self->airspeed->ladder->super.value;
        break;
      case PITCH:
        return self->attitude->super.value;
        break;
      case ROLL:
        return self->attitude->rollslip->super.value;
        break;
    }
    return NAN;
}

/*Currently the only supported location is 0,0 and width/height is 640x480*/
static void basic_hud_render(BasicHud *self, Uint32 dt, RenderTarget destination, SDL_Rect *location)
{

    base_gauge_render(BASE_GAUGE(self->attitude), dt, destination, location);
    /*Temp fix, see AttitudeIndicator::render*/
    base_gauge_render(BASE_GAUGE(self->attitude->rollslip), dt, destination, &(SDL_Rect){
        self->attitude->locations[ROLL_SLIP].x + location->x,
        self->attitude->locations[ROLL_SLIP].y + location->y,
        self->attitude->locations[ROLL_SLIP].w,
        self->attitude->locations[ROLL_SLIP].h,
    });

    base_gauge_render(BASE_GAUGE(self->altgroup), dt, destination, &self->locations[ALT_GROUP]);
    base_gauge_render(BASE_GAUGE(self->airspeed), dt, destination, &self->locations[SPEED]);
}

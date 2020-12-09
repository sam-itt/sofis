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
#include "compass-gauge.h"

static BaseGaugeOps basic_hud_ops = {
   .render = (RenderFunc)NULL,
   .update_state = (StateUpdateFunc)NULL
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
    base_gauge_init(
        BASE_GAUGE(self),
        &basic_hud_ops,
        640, /*Fixed for now, should be made dynamic and set through params*/
        480
    );

    self->attitude = attitude_indicator_new(640, 480);
    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->attitude),
        0, 0
    );

    self->altgroup = alt_group_new();
    self->altgroup->altimeter->src = ALT_SRC_GPS;
    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->altgroup),
        460, 53
    );

    self->airspeed = airspeed_indicator_new(50,60,85,155,200);
    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->airspeed),
        96, 72
    );

    self->compass = compass_gauge_new();
    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->compass),
        640/2 - base_gauge_w(BASE_GAUGE(self->compass))/2,
        (480-1) - base_gauge_h(BASE_GAUGE(self->compass))
    );


    self->locations[ALT_GROUP] = (SDL_Rect){460,53,0,0};
    self->locations[SPEED] = (SDL_Rect){96,72,0,0};
    self->locations[COMPASS] = (SDL_Rect){
        .x = 640/2 - base_gauge_w(BASE_GAUGE(self->compass))/2,
        .y = (480-1) - base_gauge_h(BASE_GAUGE(self->compass)),
        .w = base_gauge_w(BASE_GAUGE(self->compass)),
        .h = base_gauge_h(BASE_GAUGE(self->compass))
    };


    return self;
}

void basic_hud_dispose(BasicHud *self)
{
    alt_group_free(self->altgroup);
    airspeed_indicator_free(self->airspeed);
    attitude_indicator_free(self->attitude);
    compass_gauge_free(self->compass);
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
            attitude_indicator_set_pitch(self->attitude, val, true);
            break;
          case ROLL:
            attitude_indicator_set_roll(self->attitude, val, true);
            break;
          case HEADING:
            compass_gauge_set_value(self->compass, val, true);
            break;
          case HUD_VALUE_MAX: /*Fall through*/
          default:
            break;
        }
    }
}

float basic_hud_get(BasicHud *self, HudValue hv)
{
#if 0
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
      case HEADING:
        return ANIMATED_GAUGE(self->compass->compass)->value;
        break;
      case HUD_VALUE_MAX: /*Fall through*/
      default:
        break;

    }
#endif
    return NAN;
}

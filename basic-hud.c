#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "airspeed-indicator.h"
#include "alt-group.h"
#include "animated-gauge.h"
#include "attitude-indicator.h"
#include "basic-hud.h"
#include "misc.h"

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

    self->airspeed = airspeed_indicator_new( 50,60,85,155,200);
    self->attitude = attitude_indicator_new(640, 480);

    self->locations[ALT_GROUP] = (SDL_Rect){460,53,0,0};
    self->locations[SPEED] = (SDL_Rect){96,72,0,0};

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

#if 0
void basic_hud_damage_all(BasicHud *self)
{
    self->airspeed->odo->parent.damaged = true;
    self->airspeed->ladder->parent.damaged = true;

    self->altgroup->altimeter->ladder->parent.damaged = true;
    self->altgroup->altimeter->odo->parent.damaged = true;
    self->altgroup->vsi->parent.damaged = true;

    self->attitude->parent.damaged = true;
    self->attitude->rollslip->parent.damaged = true;

}
#endif

void basic_hud_set(BasicHud *self, int nvalues, ...)
{
    va_list args;

    va_start(args, nvalues);
    basic_hud_set_values(self, nvalues, args);
    va_end(args);
}

void basic_hud_set_values(BasicHud *self, uintf8_t nvalues, va_list ap)
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
//            basic_hud_damage_all(self);
            break;
          case ROLL:
            attitude_indicator_set_roll(self->attitude, -1.0*val);
//            basic_hud_damage_all(self);
            break;
        }
    }
}

float basic_hud_get(BasicHud *self, HudValue hv)
{

    switch(hv){
      case ALTITUDE:
        return self->altgroup->altimeter->ladder->parent.value;
        break;
      case VERTICAL_SPEED:
        return self->altgroup->vsi->parent.value;
        break;
      case AIRSPEED:
        return self->airspeed->ladder->parent.value;
        break;
      case PITCH:
        return self->attitude->parent.value;
        break;
      case ROLL:
        return self->attitude->rollslip->parent.value;
        break;
    }
    return NAN;
}


void basic_hud_render(BasicHud *self, Uint32 dt, SDL_Surface *destination)
{
    animated_gauge_render(ANIMATED_GAUGE(self->attitude->rollslip), dt);
    SDL_BlitSurface(animated_gauge_render(ANIMATED_GAUGE(self->attitude), dt) , NULL, destination, NULL);

    alt_group_render_at(self->altgroup, dt, destination, &self->locations[ALT_GROUP]);
    SDL_BlitSurface(airspeed_indicator_render(self->airspeed, dt), NULL, destination, &self->locations[SPEED]);
}


void basic_hud_render_to(BasicHud *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location)
{
    SDL_Rect arec, srec;

    arec = (SDL_Rect){
        location->x + self->locations[ALT_GROUP].x,
        location->y + self->locations[ALT_GROUP].y,
        0,0
    };

    srec = (SDL_Rect){
        location->x + self->locations[SPEED].x,
        location->y + self->locations[SPEED].y,
        0,0
    };


    attitude_indicator_render_to(self->attitude, dt, destination, location);
    alt_group_render_to(self->altgroup, dt, destination, &arec);
    airspeed_indicator_render_to(self->airspeed, dt, destination, &srec);

}

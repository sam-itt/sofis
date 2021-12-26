/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "airspeed-indicator.h"
#include "alt-group.h"
#include "attitude-indicator.h"
#include "base-gauge.h"
#include "basic-hud.h"
#include "misc.h"
#include "compass-gauge.h"
#include "roll-slip-gauge.h"
#include "sfv-gauge.h"
#include "tape-gauge.h"

static BaseGaugeOps basic_hud_ops = {
   .render = (RenderFunc)NULL,
   .update_state = (StateUpdateFunc)NULL,
   .dispose = (DisposeFunc)NULL
};


BasicHud *basic_hud_new(void)
{
    BasicHud *self;

    self = calloc(1, sizeof(BasicHud));
    if(self){
        if(!basic_hud_init(self)){
            return base_gauge_free(BASE_GAUGE(self));
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
          case SLIP:
            roll_slip_gauge_set_slip(self->attitude->rollslip, val, true);
            break;
          case HEADING:
            compass_gauge_set_value(self->compass, val, true);
            attitude_indicator_set_heading(self->attitude, val);
            break;
          case HUD_VALUE_MAX: /*Fall through*/
          default:
            break;
        }
    }
}

float basic_hud_get(BasicHud *self, HudValue hv)
{
    switch(hv){
      case ALTITUDE:
        return tape_gauge_get_value(self->altgroup->altimeter->tape);
        break;
      case VERTICAL_SPEED:
        return sfv_gauge_get_value(SFV_GAUGE(self->altgroup->vsi));
        break;
      case AIRSPEED:
        return tape_gauge_get_value(self->airspeed->tape);
        break;
      case PITCH:
        return self->attitude->pitch; /*will return the frame's value*/
        break;
      case ROLL:
        return sfv_gauge_get_value(SFV_GAUGE(self->attitude->rollslip));
        break;
      case HEADING:
        return sfv_gauge_get_value(SFV_GAUGE(self->compass));
        break;
      case SLIP:
        return self->attitude->rollslip->slip;
        break;
      case HUD_VALUE_MAX: /*Fall through*/
      default:
        break;

    }
    return NAN;
}

void basic_hud_attitude_changed(BasicHud *self, AttitudeData *newv)
{
    attitude_indicator_set_pitch(self->attitude, newv->pitch, true);
    attitude_indicator_set_roll(self->attitude, newv->roll, true);
    compass_gauge_set_value(self->compass, newv->heading, true);
    attitude_indicator_set_heading(self->attitude, newv->heading);
}

void basic_hud_dynamics_changed(BasicHud *self, DynamicsData *newv)
{
//    printf("called with airpseed: %f, vertical speed: %f\n", newv->airspeed, newv->vertical_speed*60);
    airspeed_indicator_set_value(self->airspeed, newv->airspeed);
    /* Convert fps to fpm
     * TODO: Document and make units consistent
     * */
    alt_group_set_vertical_speed(self->altgroup, newv->vertical_speed * 60);
    roll_slip_gauge_set_slip(self->attitude->rollslip, newv->slip_rad * 180.0/M_PI, true);
}

void basic_hud_location_changed(BasicHud *self, LocationData *newv)
{
    alt_group_set_altitude(self->altgroup, newv->altitude);
}


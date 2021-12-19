/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include "SDL_pcf.h"

#include "airspeed-indicator.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "misc.h"

static BaseGaugeOps airspeed_indicator_ops = {
   .render = (RenderFunc)NULL,
   .update_state = (StateUpdateFunc)NULL,
   .dispose = (DisposeFunc)NULL
};


AirspeedIndicator *airspeed_indicator_new(speed_t v_so, speed_t v_s1, speed_t v_fe, speed_t v_no, speed_t v_ne)
{
    AirspeedIndicator *self;

    self = calloc(1, sizeof(AirspeedIndicator));
    if(self){
        if(!airspeed_indicator_init(self, v_so, v_s1, v_fe, v_no, v_ne)){
            return base_gauge_dispose(BASE_GAUGE(self));
        }
    }
    return self;
}


AirspeedIndicator *airspeed_indicator_init(AirspeedIndicator *self, speed_t v_so, speed_t v_s1, speed_t v_fe, speed_t v_no, speed_t v_ne)
{
    PCF_Font *fnt;
    DigitBarrel *db;

    base_gauge_init(BASE_GAUGE(self), &airspeed_indicator_ops, 68, 240+20);

    fnt = resource_manager_get_font(TERMINUS_18);
    db = digit_barrel_new(fnt, 0, 9.999, 1);
    self->tape = tape_gauge_new(
        (LadderPageDescriptor*)airspeed_page_descriptor_new(v_so,  v_s1,  v_fe,  v_no,  v_ne),
        AlignRight, -12, 3,
        -1, db,
        -2, db,
        -2, db
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->tape), 0, 0);

    self->txt = text_gauge_new(NULL, true, 68, 21);
    self->txt->alignment = HALIGN_CENTER | VALIGN_MIDDLE;
    text_gauge_set_static_font(self->txt,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, "TASKT-", PCF_DIGITS
        )
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->txt),
        0,
        base_gauge_h(BASE_GAUGE(self)) - 20 - 1
    );
    text_gauge_set_color(self->txt, SDL_BLACK, BACKGROUND_COLOR);

    airspeed_indicator_set_value(self, 0.0);
    return self;
}

bool airspeed_indicator_set_value(AirspeedIndicator *self, float value)
{
    float cad; /*Current air density, must be in kg/m3 (same unit as RHO_0)*/

    cad = RHO_0; /*Placeholder, should be reported by a sensor*/

    self->tas = round(value * sqrt(RHO_0/cad));
    text_gauge_set_value_formatn(self->txt,
        9, /*TAS XXXKT*/
        "TAS %03dKT", self->tas
    );

    tape_gauge_set_value(self->tape, value, true);

    return true;
}

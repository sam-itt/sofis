#include <stdio.h>
#include <stdlib.h>

#include "SDL_pcf.h"

#include "airspeed-indicator.h"
#include "airspeed-page-descriptor.h"
#include "base-animation.h"
#include "base-gauge.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "misc.h"

#define SPIN_DURATION 1000

#if 0
static void airspeed_indicator_render(AirspeedIndicator *self, Uint32 dt);
static BufferedGaugeOps airspeed_indicator_ops = {
    .render = (BufferRenderFunc)airspeed_indicator_render
};
#endif
static void airspeed_indicator_update_state(AirspeedIndicator *self, Uint32 dt);
static BaseGaugeOps airspeed_indicator_ops = {
   .render = (RenderFunc)NULL,
   .update_state = (StateUpdateFunc)airspeed_indicator_update_state
};


AirspeedIndicator *airspeed_indicator_new(speed_t v_so, speed_t v_s1, speed_t v_fe, speed_t v_no, speed_t v_ne)
{
    AirspeedIndicator *self;

    self = calloc(1, sizeof(AirspeedIndicator));
    if(self){
        if(!airspeed_indicator_init(self, v_so, v_s1, v_fe, v_no, v_ne)){
            free(self);
            return NULL;
        }
    }
    return self;
}


AirspeedIndicator *airspeed_indicator_init(AirspeedIndicator *self, speed_t v_so, speed_t v_s1, speed_t v_fe, speed_t v_no, speed_t v_ne)
{
    AirspeedPageDescriptor *descriptor;
    DigitBarrel *db;
    PCF_Font *fnt;

    base_gauge_init(BASE_GAUGE(self), &airspeed_indicator_ops, 68, 240+20);

    descriptor = airspeed_page_descriptor_new( v_so,  v_s1,  v_fe,  v_no,  v_ne);
    self->ladder = ladder_gauge_new((LadderPageDescriptor *)descriptor, -1);
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->ladder), 0, 0);

    fnt = resource_manager_get_font(TERMINUS_18);
    db = digit_barrel_new(fnt, 0, 9.999, 1);
    self->odo = odo_gauge_new_multiple(-1, 3,
            -1, db,
            -2, db,
            -2, db
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->odo),
        25,
        (base_gauge_h(BASE_GAUGE(self->ladder))-1)/2.0
         - base_gauge_h(BASE_GAUGE(self->odo))/2.0 + 1
    );

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

void airspeed_indicator_dispose(AirspeedIndicator *self)
{
    ladder_gauge_free(self->ladder);
    odo_gauge_free(self->odo);
    text_gauge_free(self->txt);
    base_gauge_dispose(BASE_GAUGE(self));
}

void airspeed_indicator_free(AirspeedIndicator *self)
{
    airspeed_indicator_dispose(self);
    free(self);
}

bool airspeed_indicator_set_value(AirspeedIndicator *self, float value)
{
    float cad; /*Current air density, must be in kg/m3 (same unit as RHO_0)*/
    char number[10]; //TAS XXXKT plus \0

    cad = RHO_0; /*Placeholder, should be reported by a sensor*/

    self->tas = round(value * sqrt(RHO_0/cad));
    snprintf(number, 10, "TAS %03dKT", self->tas);
    text_gauge_set_value(self->txt, number);

    /*TODO after refactor: Create TapeGauge*/
    BaseAnimation *animation;
    if(BASE_GAUGE(self)->nanimations == 0){
        animation = base_animation_new(TYPE_FLOAT, 2,
            &self->ladder->value,
            &self->odo->value
        );
        base_gauge_add_animation(BASE_GAUGE(self), animation);
        base_animation_unref(animation);/*base_gauge takes ownership*/
    }else{
        animation = BASE_GAUGE(self)->animations[0];
    }
    base_animation_start(animation, self->ladder->value, value, SPIN_DURATION);

    return true;
}

static void airspeed_indicator_update_state(AirspeedIndicator *self, Uint32 dt)
{
    BaseAnimation *animation;

    if(BASE_GAUGE(self)->nanimations > 0){
        animation = BASE_GAUGE(self)->animations[0];
        if(!animation->finished){
            BASE_GAUGE(self->ladder)->dirty = true;
            BASE_GAUGE(self->odo)->dirty = true;
        }
    }
}

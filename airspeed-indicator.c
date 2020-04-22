#include <stdio.h>
#include <stdlib.h>

#include "SDL_surface.h"
#include "airspeed-indicator.h"
#include "airspeed-page-descriptor.h"
#include "base-gauge.h"
#include "buffered-gauge.h"
#include "sdl-colors.h"
#include "view.h"

static void airspeed_indicator_render(AirspeedIndicator *self, Uint32 dt);
static BufferedGaugeOps airspeed_indicator_ops = {
    .render = (BufferRenderFunc)airspeed_indicator_render
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

    buffered_gauge_init(BUFFERED_GAUGE(self), &airspeed_indicator_ops, 68, 240+20);

    descriptor = airspeed_page_descriptor_new( v_so,  v_s1,  v_fe,  v_no,  v_ne);
    self->ladder = ladder_gauge_new((LadderPageDescriptor *)descriptor, -1);
    buffered_gauge_set_buffer(
        BUFFERED_GAUGE(self->ladder),
        buffered_gauge_get_view(BUFFERED_GAUGE(self)),
        0,
        0
    );

    db = digit_barrel_new(18, 0, 9.999, 1);
    self->odo = odo_gauge_new_multiple(-1, 3,
            -1, db,
            -2, db,
            -2, db
    );
    buffered_gauge_set_buffer(
        BUFFERED_GAUGE(self->odo),
        buffered_gauge_get_view(BUFFERED_GAUGE(self->ladder)),
        25,
        (BASE_GAUGE(self->ladder)->h-1)/2.0 - BASE_GAUGE(self->odo)->h/2.0 +1
    );

    buffered_gauge_clear(BUFFERED_GAUGE(self),NULL);
    self->font  = TTF_OpenFont("TerminusTTF-4.47.0.ttf", 12);

    return self;
}

void airspeed_indicator_dispose(AirspeedIndicator *self)
{
    ladder_gauge_free(self->ladder);
    odo_gauge_free(self->odo);
    buffered_gauge_dispose(BUFFERED_GAUGE(self));
    TTF_CloseFont(self->font);
}

void airspeed_indicator_free(AirspeedIndicator *self)
{
    airspeed_indicator_dispose(self);
    free(self);
}

bool airspeed_indicator_set_value(AirspeedIndicator *self, float value)
{
    float cad; /*Current air density, must be in kg/m3 (same unit as RHO_0)*/

    cad = RHO_0; /*Placeholder, should be reported by a sensor*/

    self->tas = round(value * sqrt(RHO_0/cad));

    animated_gauge_set_value(ANIMATED_GAUGE(self->ladder), value);
    BUFFERED_GAUGE(self)->damaged = true;
    return odo_gauge_set_value(self->odo, value);
}

static void airspeed_indicator_draw_tas(AirspeedIndicator *self)
{
    char number[10]; //TAS XXXKT plus \0
    SDL_Rect location, oloc;

    oloc = (SDL_Rect){
        .x = 0,
        .y = BASE_GAUGE(self)->h - 20 - 1,
        .h = 20,
        .w = BASE_GAUGE(self)->w
    };

    buffered_gauge_draw_outline(BUFFERED_GAUGE(self), &SDL_WHITE, &oloc);

    location.x = oloc.x + 1;
    location.y = oloc.y + 1;
    location.h = oloc.h;
    location.w = oloc.w-2;

    snprintf(number, 10, "TAS %03dKT", self->tas);
    buffered_gauge_draw_text(BUFFERED_GAUGE(self), &location, number, self->font, &SDL_WHITE, &SDL_BLACK);
}


static void airspeed_indicator_render(AirspeedIndicator *self, Uint32 dt)
{
    buffered_gauge_clear(BUFFERED_GAUGE(self), NULL);
    airspeed_indicator_draw_tas(self);
    buffered_gauge_draw_outline(BUFFERED_GAUGE(self), &SDL_WHITE, NULL);

    buffered_gauge_paint_buffer(BUFFERED_GAUGE(self->ladder), dt);
    buffered_gauge_paint_buffer(BUFFERED_GAUGE(self->odo), dt);
    if(animated_gauge_moving(ANIMATED_GAUGE(self->ladder)) || animated_gauge_moving(ANIMATED_GAUGE(self->odo)))
        BUFFERED_GAUGE(self)->damaged = true;
}

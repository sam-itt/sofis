#include <stdio.h>
#include <stdlib.h>

#include "SDL_surface.h"
#include "airspeed-indicator.h"
#include "airspeed-page-descriptor.h"
#include "base-gauge.h"
#include "sdl-colors.h"
#include "view.h"

static SDL_Surface *airspeed_indicator_render(AirspeedIndicator *self, Uint32 dt);
static void airspeed_indicator_render_to(AirspeedIndicator *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);
static BaseGaugeOps airspeed_indicator_ops = {
    .render = (RenderFunc)airspeed_indicator_render,
    .render_to = (RenderToFunc)airspeed_indicator_render_to
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

    descriptor = airspeed_page_descriptor_new( v_so,  v_s1,  v_fe,  v_no,  v_ne);
    self->ladder = ladder_gauge_new((LadderPageDescriptor *)descriptor, -1);

    db = digit_barrel_new(18, 0, 9.999, 1);
    self->odo = odo_gauge_new_multiple(-1, 3,
            -1, db,
            -2, db,
            -2, db
    );

    base_gauge_init(
        BASE_GAUGE(self),
        &airspeed_indicator_ops,
        BASE_GAUGE(self->ladder)->w,
        BASE_GAUGE(self->ladder)->h + 20
    );

    self->view = SDL_CreateRGBSurfaceWithFormat(0,
        BASE_GAUGE(self)->w,
        BASE_GAUGE(self)->h,
        32, SDL_PIXELFORMAT_RGBA32
    );
    SDL_SetColorKey(self->view, SDL_TRUE, SDL_UCKEY(self->view));
    SDL_FillRect(self->view, NULL, SDL_UCKEY(self->view));
    self->w = ANIMATED_GAUGE(self->ladder)->view->w;
    self->h = ANIMATED_GAUGE(self->ladder)->view->h + 20;
    self->font  = TTF_OpenFont("TerminusTTF-4.47.0.ttf", 12);

    return self;
}

void airspeed_indicator_dispose(AirspeedIndicator *self)
{
    ladder_gauge_free(self->ladder);
    odo_gauge_free(self->odo);
    SDL_FreeSurface(self->view);
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

    view_draw_outline(self->view, &(SDL_WHITE), &oloc);


    location.x = oloc.x + 1;
    location.y = oloc.y + 1;
    location.h = oloc.h;
    location.w = oloc.w-2;

    snprintf(number, 10, "TAS %03dKT", self->tas);
    view_draw_text(self->view, &location, number, self->font, &SDL_WHITE, &SDL_BLACK);
}


static SDL_Surface *airspeed_indicator_render(AirspeedIndicator *self, Uint32 dt)
{
    SDL_Surface *lad;
    SDL_Surface *odo;
    SDL_Rect placement[2];

    if(animated_gauge_moving(ANIMATED_GAUGE(self->ladder)) || animated_gauge_moving(ANIMATED_GAUGE(self->odo))){
        memset(placement, 0, sizeof(SDL_Rect)*2);
        placement[0].y = 0;

        view_clear(self->view);
        airspeed_indicator_draw_tas(self);
        view_draw_outline(self->view, &(SDL_WHITE), NULL);

        lad = base_gauge_render(BASE_GAUGE(self->ladder), dt);
        odo = base_gauge_render(BASE_GAUGE(self->odo), dt);
        SDL_BlitSurface(lad, NULL, self->view, &placement[0]);

        placement[1].y = placement[0].y + (lad->h-1)/2.0 - odo->h/2.0 +1;
        placement[1].x = 25;// (lad->w - odo->w-1) -1; /*The last -1 in there to prevent eating the border*/
        SDL_BlitSurface(odo, NULL, self->view, &placement[1]);
    }
    return self->view;
}

static void airspeed_indicator_draw_tas_to(AirspeedIndicator *self, SDL_Surface *destination, SDL_Rect *location)
{
    char number[10]; //TAS XXXKT plus \0
    SDL_Rect tloc, oloc;

    oloc = (SDL_Rect){
        .x = location->x,
        .y = location->y + BASE_GAUGE(self)->h - 20 - 1,
        .h = 20,
        .w = BASE_GAUGE(self)->w
    };

    view_draw_outline(destination, &(SDL_WHITE), &oloc);

    tloc.x = oloc.x + 1;
    tloc.y = oloc.y + 1;
    tloc.h = oloc.h;
    tloc.w = oloc.w-2;

    snprintf(number, 10, "TAS %03dKT", self->tas);
    view_draw_text(destination, &tloc, number, self->font, &SDL_WHITE, &SDL_BLACK);
}


static void airspeed_indicator_render_to(AirspeedIndicator *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location)
{
    SDL_Rect placement[2];

//    if(animated_gauge_moving(ANIMATED_GAUGE(self->ladder)) || animated_gauge_moving(ANIMATED_GAUGE(self->odo))){
        memset(placement, 0, sizeof(SDL_Rect)*2);
        placement[0].y = location->y;
        placement[0].x = location->x;

        /*Clear the area before drawing. TODO, move upper in animated_gauge ?*/
//        SDL_FillRect(destination, &(SDL_Rect){location->x,location->y,self->w,self->h}, SDL_UFBLUE(destination));
        SDL_Rect area = {
            location->x, location->y,
            BASE_GAUGE(self)->w,BASE_GAUGE(self)->h
        };
        airspeed_indicator_draw_tas_to(self, destination, location);
        view_draw_outline(destination, &(SDL_WHITE), &area);

        base_gauge_render_to(BASE_GAUGE(self->ladder), dt, destination, &placement[0]);

        placement[1].y = placement[0].y + (BASE_GAUGE(self->ladder)->h-1)/2.0 - BASE_GAUGE(self->odo)->h/2.0 +1;
        placement[1].x = location->x + 25;// (lad->w - odo->w-1) -1; /*The last -1 in there to prevent eating the border*/

        base_gauge_render_to(BASE_GAUGE(self->odo), dt, destination, &placement[1]);
//    }
}

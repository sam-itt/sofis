#include <stdio.h>
#include <stdlib.h>

#include "airspeed-indicator.h"
#include "airspeed-page-descriptor.h"
#include "sdl-colors.h"


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

    self->view = SDL_CreateRGBSurfaceWithFormat(0,
        ANIMATED_GAUGE(self->ladder)->view->w,
        ANIMATED_GAUGE(self->ladder)->view->h + 20,
        32, SDL_PIXELFORMAT_RGBA32
    );
    SDL_SetColorKey(self->view, SDL_TRUE, SDL_UCKEY(self->view));
    SDL_FillRect(self->view, NULL, SDL_UCKEY(self->view));
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

static void airspeed_indicator_draw_outline(AirspeedIndicator *self)
{
    int x,y;
    SDL_Surface *gauge;

    gauge = self->view;

    SDL_LockSurface(gauge);
    Uint32 *pixels = gauge->pixels;
    Uint32 color = SDL_UWHITE(gauge);
    y = 0;
    for(x = 0; x < gauge->w; x++){
        pixels[y * gauge->w + x] = color;
    }
    y = gauge->h-1 - 20;
    for(x = 0; x < gauge->w; x++){
        pixels[y * gauge->w + x] = color;
    }
    y = gauge->h - 1;
    for(x = 0; x < gauge->w; x++){
        pixels[y * gauge->w + x] = color;
    }
    x = gauge->w - 1;
    for(y = 0; y < gauge->h; y++){
        pixels[y * gauge->w + x] = color;
    }
    x = 0;
    for(y = 0; y < gauge->h; y++){
        pixels[y * gauge->w + x] = color;
    }

    SDL_UnlockSurface(gauge);
}



static void airspeed_indicator_draw_text(AirspeedIndicator *self, const char *string, SDL_Rect *location, SDL_Color *color)
{
    SDL_Surface *text;

    SDL_FillRect(self->view, location, SDL_UBLACK(self->view));

    text = TTF_RenderText_Solid(self->font, string, *color);

    location->x = round(location->w/2.0) - round(text->w/2.0);
    location->y += round(location->h/2.0) - round(text->h/2.0);
    SDL_BlitSurface(text, NULL, self->view, location);
    SDL_FreeSurface(text);
}


static void airspeed_indicator_draw_tas(AirspeedIndicator *self)
{
    char number[10]; //TAS XXXKT plus \0
    SDL_Rect location;

    location.x = 1;
    location.y = self->view->h-1 - 20;
    location.h = 20;
    location.w = self->view->w-2; //-2 to account for the outline ?

    snprintf(number, 10, "TAS %03dKT", self->tas);
    airspeed_indicator_draw_text(self, number, &location, &SDL_WHITE);
}


SDL_Surface *airspeed_indicator_render(AirspeedIndicator *self, Uint32 dt)
{
    SDL_Surface *lad;
    SDL_Surface *odo;
    SDL_Rect placement[2];

    if(animated_gauge_moving(ANIMATED_GAUGE(self->ladder)) || animated_gauge_moving(ANIMATED_GAUGE(self->odo))){
        memset(placement, 0, sizeof(SDL_Rect)*2);
        placement[0].y = 0;

        SDL_FillRect(self->view, NULL, SDL_UCKEY(self->view));

        airspeed_indicator_draw_tas(self);
        airspeed_indicator_draw_outline(self);

        lad = base_gauge_render(BASE_GAUGE(self->ladder), dt);
        odo = base_gauge_render(BASE_GAUGE(self->odo), dt);
        SDL_BlitSurface(lad, NULL, self->view, &placement[0]);

        placement[1].y = placement[0].y + (lad->h-1)/2.0 - odo->h/2.0 +1;
        placement[1].x = 25;// (lad->w - odo->w-1) -1; /*The last -1 in there to prevent eating the border*/
        SDL_BlitSurface(odo, NULL, self->view, &placement[1]);
    }
    return self->view;
}

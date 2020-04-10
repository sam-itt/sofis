#include <stdio.h>
#include <stdlib.h>

#include "alt-indicator.h"
#include "alt-ladder-page-descriptor.h"
#include "sdl-colors.h"

AltIndicator *alt_indicator_new(void)
{
    AltIndicator *self;
    self = calloc(1, sizeof(AltIndicator));
    if(self){
        self->ladder = ladder_gauge_new((LadderPageDescriptor *)alt_ladder_page_descriptor_new(), -1);

        DigitBarrel *db = digit_barrel_new(18, 0, 9.999, 1);
        DigitBarrel *db2 = digit_barrel_new(18, 0, 99, 10);
        self->odo = odo_gauge_new_multiple(-1, 4,
                -1, db2,
                -2, db,
                -2, db,
                -2, db
        );

        self->view = SDL_CreateRGBSurfaceWithFormat(0,
            ANIMATED_GAUGE(self->ladder)->view->w,
            ANIMATED_GAUGE(self->ladder)->view->h + 20 * 2,
            32, SDL_PIXELFORMAT_RGBA32
        );
        SDL_SetColorKey(self->view, SDL_TRUE, SDL_UCKEY(self->view));
        SDL_FillRect(self->view, NULL, SDL_UCKEY(self->view));
        self->font  = TTF_OpenFont("TerminusTTF-4.47.0.ttf", 16);
    }
    return self;
}

void alt_indicator_free(AltIndicator *self)
{
    ladder_gauge_free(self->ladder);
    odo_gauge_free(self->odo);
    TTF_CloseFont(self->font);
    SDL_FreeSurface(self->view);
    free(self);
}

bool alt_indicator_set_value(AltIndicator *self, float value)
{

    animated_gauge_set_value(ANIMATED_GAUGE(self->ladder), value);
    return odo_gauge_set_value(self->odo, value);
}

/*HPa*/
void alt_indicator_set_qnh(AltIndicator *self, float value)
{
    /* This is temporary during gfx dev. The real feature will damage
     * the gauge by changing the actual value*/
    if(self->qnh != value){
        self->qnh = value;
        ANIMATED_GAUGE(self->ladder)->damaged = true;
    }
}


static void alt_indicator_draw_outline(AltIndicator *self)
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
    y= 19;
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
    /*TODO: don't draw left side if current value > page 1 limit*/
    x = 0;
    for(y = 0; y < gauge->h; y++){
        pixels[y * gauge->w + x] = color;
    }

    SDL_UnlockSurface(gauge);
}


static void alt_indicator_draw_text(AltIndicator *self, const char *string, SDL_Rect *location, SDL_Color *color)
{
    SDL_Surface *text;

    SDL_FillRect(self->view, location, SDL_UBLACK(self->view));

    text = TTF_RenderText_Solid(self->font, string, *color);

    location->x = round(location->w/2.0) - round(text->w/2.0);
    SDL_BlitSurface(text, NULL, self->view, location);
    SDL_FreeSurface(text);
}

static void alt_indicator_draw_qnh(AltIndicator *self)
{
    char number[6]; //5 digits plus \0
    SDL_Rect location;
    SDL_Color color;

    location.x = 1;
    location.y = self->view->h-1 - 20;
    location.h = 20;
    location.w = self->view->w-2; //-2 to account for the outline ?


    if(self->src != ALT_SRC_GPS){
        snprintf(number, 6, "%0.2f", self->qnh);
        color = SDL_WHITE;
    }else{
        snprintf(number, 6, "GPS");
        color = SDL_RED;
    }
    alt_indicator_draw_text(self, number, &location, &color);
}

static void alt_indicator_draw_target_altitude(AltIndicator *self)
{
    char number[6]; //5 digits plus \0
    SDL_Rect location;

    location.x = 1;
    location.y = 1;
    location.h = 19;
    location.w = self->view->w-2; //-2 to account for the outline ?

    if(self->target_alt >= 0){
        snprintf(number, 6, "%d", self->target_alt);
    }else{
        snprintf(number, 6, "----");
    }
    alt_indicator_draw_text(self, number, &location, &SDL_WHITE);
}



SDL_Surface *alt_indicator_render(AltIndicator *self, Uint32 dt)
{
    SDL_Surface *lad;
    SDL_Surface *odo;
    SDL_Rect placement[2];

    if(animated_gauge_moving(ANIMATED_GAUGE(self->ladder)) || animated_gauge_moving(ANIMATED_GAUGE(self->odo))){
        memset(placement, 0, sizeof(SDL_Rect)*2);
        placement[0].y = 19;


        SDL_FillRect(self->view, NULL, SDL_UCKEY(self->view));

        alt_indicator_draw_qnh(self);
        alt_indicator_draw_target_altitude(self);
        alt_indicator_draw_outline(self);

        lad = animated_gauge_render(ANIMATED_GAUGE(self->ladder), dt);
        odo = animated_gauge_render(ANIMATED_GAUGE(self->odo), dt);
        SDL_BlitSurface(lad, NULL, self->view, &placement[0]);

        placement[1].y = placement[0].y + (lad->h-1)/2.0 - odo->h/2.0 +1;
        placement[1].x = (lad->w - odo->w-1) -1; /*The last -1 in there to prevent eating the border*/
        SDL_BlitSurface(odo, NULL, self->view, &placement[1]);
    }
    return self->view;
}

uint32_t alt_indicator_get_width(AltIndicator *self)
{
//    return ANIMATED_GAUGE(self->ladder)->view->w;
    return self->view->w;
}

uint32_t alt_indicator_get_height(AltIndicator *self)
{
    //return ANIMATED_GAUGE(self->ladder)->view->h;
    return self->view->h;
}

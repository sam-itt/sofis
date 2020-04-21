#include <stdio.h>
#include <stdlib.h>

#include "SDL_rect.h"
#include "alt-indicator.h"
#include "alt-ladder-page-descriptor.h"
#include "animated-gauge.h"
#include "base-gauge.h"
#include "sdl-colors.h"

static void alt_indicator_render(AltIndicator *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);
static BaseGaugeOps alt_indicator_ops = {
    .render = (RenderFunc)alt_indicator_render
};


AltIndicator *alt_indicator_new(void)
{
    AltIndicator *self;
    self = calloc(1, sizeof(AltIndicator));
    if(self){
        if(!alt_indicator_init(self)){
            free(self);
            return NULL;
        }
    }
    return self;
}

AltIndicator *alt_indicator_init(AltIndicator *self)
{
    self->ladder = ladder_gauge_new((LadderPageDescriptor *)alt_ladder_page_descriptor_new(), -1);

    DigitBarrel *db = digit_barrel_new(18, 0, 9.999, 1);
    DigitBarrel *db2 = digit_barrel_new(18, 0, 99, 10);
    self->odo = odo_gauge_new_multiple(-1, 4,
            -1, db2,
            -2, db,
            -2, db,
            -2, db
    );
    base_gauge_init(
        BASE_GAUGE(self),
        &alt_indicator_ops,
        BASE_GAUGE(self->ladder)->w,
        BASE_GAUGE(self->ladder)->h + 20 * 2
    );

    self->view = SDL_CreateRGBSurfaceWithFormat(0,
        BASE_GAUGE(self)->w,
        BASE_GAUGE(self)->h,
        32, SDL_PIXELFORMAT_RGBA32
    );
    SDL_SetColorKey(self->view, SDL_TRUE, SDL_UCKEY(self->view));
    SDL_FillRect(self->view, NULL, SDL_UCKEY(self->view));
    self->font  = TTF_OpenFont("TerminusTTF-4.47.0.ttf", 16);

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

static void alt_indicator_draw_qnh(AltIndicator *self)
{
    char number[6]; //5 digits plus \0
    SDL_Rect location, oloc;
    SDL_Color color;

    oloc = (SDL_Rect){
        .x = 0,
        .y = BASE_GAUGE(self)->h - 20 -2,
        .h = 21,
        .w = BASE_GAUGE(self)->w
    };

    view_draw_outline(self->view, &(SDL_WHITE), &oloc);

    location.x = oloc.x + 1;
    location.y = oloc.y + 1;
    location.h = oloc.h - 1;
    location.w = oloc.w - 2;


    if(self->src != ALT_SRC_GPS){
        snprintf(number, 6, "%0.2f", self->qnh);
        color = SDL_WHITE;
    }else{
        snprintf(number, 6, "GPS");
        color = SDL_RED;
    }
    view_draw_text(self->view, &location, number, self->font, &color, &SDL_BLACK);
}

static void alt_indicator_draw_target_altitude(AltIndicator *self)
{
    char number[6]; //5 digits plus \0
    SDL_Rect location, oloc;

    oloc = (SDL_Rect){
        .x = 0,
        .y = 0,
        .h = 20,
        .w = BASE_GAUGE(self)->w
    };

    view_draw_outline(self->view, &(SDL_WHITE), &oloc);

    location.x = oloc.x + 1;
    location.y = oloc.y + 1;
    location.h = oloc.h - 1;
    location.w = oloc.w - 2;

    if(self->target_alt >= 0){
        snprintf(number, 6, "%d", self->target_alt);
    }else{
        snprintf(number, 6, "----");
    }
    view_draw_text(self->view, &location, number, self->font, &SDL_WHITE, &SDL_BLACK);
}



static void alt_indicator_render(AltIndicator *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location)
{
    SDL_Rect placement[2];

    if(animated_gauge_moving(ANIMATED_GAUGE(self->ladder)) || animated_gauge_moving(ANIMATED_GAUGE(self->odo))){
        memset(placement, 0, sizeof(SDL_Rect)*2);
        placement[0].y = 19;

        view_clear(self->view);
        alt_indicator_draw_qnh(self);
        alt_indicator_draw_target_altitude(self);
        view_draw_outline(self->view, &(SDL_WHITE), NULL);

        base_gauge_render(BASE_GAUGE(self->ladder), dt, self->view, &placement[0]);

        placement[1].y = placement[0].y + (BASE_GAUGE(self->ladder)->h-1)/2.0 - BASE_GAUGE(self->odo)->h/2.0 +1;
        placement[1].x = (BASE_GAUGE(self->ladder)->w - BASE_GAUGE(self->odo)->w-1) -1; /*The last -1 in there to prevent eating the border*/
        base_gauge_render(BASE_GAUGE(self->odo), dt, self->view, &placement[1]);
    }
    SDL_BlitSurface(self->view, NULL, destination, location);
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

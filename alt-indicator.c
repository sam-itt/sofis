#include <stdio.h>
#include <stdlib.h>

#include "SDL_rect.h"
#include "alt-indicator.h"
#include "alt-ladder-page-descriptor.h"
#include "animated-gauge.h"
#include "base-gauge.h"
#include "buffered-gauge.h"
#include "misc.h"
#include "sdl-colors.h"

static void alt_indicator_render(AltIndicator *self, Uint32 dt);
static BufferedGaugeOps alt_indicator_ops = {
    .render = (BufferRenderFunc)alt_indicator_render
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
    buffered_gauge_init(BUFFERED_GAUGE(self), &alt_indicator_ops, 68, 240+20);

    /*TODO: Change size to size - 20, when size becomes a parameter !
     * temporary fixed in the rendering function by drawing the ladder first
     * and then drawing on it
     * */
    self->ladder = ladder_gauge_new((LadderPageDescriptor *)alt_ladder_page_descriptor_new(), -1);
    buffered_gauge_set_buffer(
        BUFFERED_GAUGE(self->ladder),
        buffered_gauge_get_view(BUFFERED_GAUGE(self)),
        0,
        19
    );

    DigitBarrel *db = digit_barrel_new(18, 0, 9.999, 1);
    DigitBarrel *db2 = digit_barrel_new(18, 0, 99, 10);
    self->odo = odo_gauge_new_multiple(-1, 4,
            -1, db2,
            -2, db,
            -2, db,
            -2, db
    );
    buffered_gauge_set_buffer(
        BUFFERED_GAUGE(self->odo),
        buffered_gauge_get_view(BUFFERED_GAUGE(self->ladder)),
        (BASE_GAUGE(self->ladder)->w - BASE_GAUGE(self->odo)->w-1) -1,/*The last -1 in there to prevent eating the border*/
        19 + (BASE_GAUGE(self->ladder)->h-1)/2.0 - BASE_GAUGE(self->odo)->h/2.0 +1
    );

    buffered_gauge_clear(BUFFERED_GAUGE(self),NULL);
    self->font  = TTF_OpenFont("TerminusTTF-4.47.0.ttf", 16);

    return self;
}

void alt_indicator_free(AltIndicator *self)
{
    ladder_gauge_free(self->ladder);
    odo_gauge_free(self->odo);
    TTF_CloseFont(self->font);
    buffered_gauge_dispose(BUFFERED_GAUGE(self));
    free(self);
}

bool alt_indicator_set_value(AltIndicator *self, float value)
{

    animated_gauge_set_value(ANIMATED_GAUGE(self->ladder), value);
    BUFFERED_GAUGE(self)->damaged = true;
    return odo_gauge_set_value(self->odo, value);
}

/*HPa*/
void alt_indicator_set_qnh(AltIndicator *self, float value)
{
    /* This is temporary during gfx dev. The real feature will damage
     * the gauge by changing the actual value*/
    if(self->qnh != value){
        self->qnh = value;
        BUFFERED_GAUGE(self->ladder)->damaged = true;
        BUFFERED_GAUGE(self)->damaged = true; /*Will be replaced by a global system*/
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

    buffered_gauge_draw_outline(BUFFERED_GAUGE(self), &(SDL_WHITE), &oloc);

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
    buffered_gauge_draw_text(BUFFERED_GAUGE(self), &location, number, self->font, &color, &SDL_BLACK);
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

    buffered_gauge_draw_outline(BUFFERED_GAUGE(self),&(SDL_WHITE), &oloc);

    location.x = oloc.x + 1;
    location.y = oloc.y + 1;
    location.h = oloc.h - 1;
    location.w = oloc.w - 2;

    if(self->target_alt >= 0){
        snprintf(number, 6, "%d", self->target_alt);
    }else{
        snprintf(number, 6, "----");
    }
    buffered_gauge_draw_text(BUFFERED_GAUGE(self), &location, number, self->font, &SDL_WHITE, &SDL_BLACK);
}



static void alt_indicator_render(AltIndicator *self, Uint32 dt)
{
    buffered_gauge_clear(BUFFERED_GAUGE(self), NULL);
    buffered_gauge_paint_buffer(BUFFERED_GAUGE(self->ladder), dt);
    alt_indicator_draw_qnh(self);
    alt_indicator_draw_target_altitude(self);
    buffered_gauge_draw_outline(BUFFERED_GAUGE(self), &SDL_WHITE, NULL);

    buffered_gauge_paint_buffer(BUFFERED_GAUGE(self->odo), dt);
    if(animated_gauge_moving(ANIMATED_GAUGE(self->ladder)) || animated_gauge_moving(ANIMATED_GAUGE(self->odo)))
        BUFFERED_GAUGE(self)->damaged = true;
}

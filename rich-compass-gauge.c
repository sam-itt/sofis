#include <stdio.h>
#include <stdlib.h>

#include "SDL_pcf.h"
#include "animated-gauge.h"
#include "base-gauge.h"
#include "compass-gauge.h"
#include "misc.h"
#include "resource-manager.h"
#include "rich-compass-gauge.h"
#include "sdl-colors.h"
#include "text-gauge.h"

static void rich_compass_gauge_render(RichCompassGauge *self, Uint32 dt, RenderTarget destination, SDL_Rect *location);
static BaseGaugeOps rich_compass_gauge_ops = {
    .render = (RenderFunc)rich_compass_gauge_render
};

RichCompassGauge *rich_compass_gauge_new(void)
{
    RichCompassGauge *rv;
    rv = calloc(1, sizeof(RichCompassGauge));
    if(rv){
        if(!rich_compass_gauge_init(rv)){
            free(rv);
            return NULL;
        }
    }
    return rv;
}

RichCompassGauge *rich_compass_gauge_init(RichCompassGauge *self)
{
    self->compass = compass_gauge_new();
    self->caption = text_gauge_new("000°", true, 28, 12);
    text_gauge_set_color(self->caption, SDL_BLACK, BACKGROUND_COLOR);
    self->caption->alignment = HALIGN_CENTER | VALIGN_MIDDLE;
    text_gauge_set_static_font(self->caption,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE, 2, PCF_DIGITS, "°")
    );

    base_gauge_init(
        BASE_GAUGE(self),
        &rich_compass_gauge_ops,
        BASE_GAUGE(self->compass)->w, /*Fixed for now, should be made dynamic and set through params*/
        BASE_GAUGE(self->compass)->h + BASE_GAUGE(self->caption)->h
    );


    return self;
}

void rich_compass_gauge_dispose(RichCompassGauge *self)
{
    if(self->compass)
        compass_gauge_free(self->compass);
    if(self->caption)
        text_gauge_free(self->caption);
}

void rich_compass_gauge_free(RichCompassGauge *self)
{
    rich_compass_gauge_dispose(self);
    free(self);
}

#define rect_offset(r1, r2) ((SDL_Rect){(r1)->x + (r2)->x, (r1)->y + (r2)->y, (r1)->w, (r1)->h})
static void rich_compass_gauge_render(RichCompassGauge *self, Uint32 dt, RenderTarget destination, SDL_Rect *location)
{
    SDL_Rect text_loc;
    SDL_Rect compass_loc;
    char value[5]; //3 digits, degree sign and '\0'


    compass_loc = (SDL_Rect){
        .x = 0,
        .y = 0 + BASE_GAUGE(self->caption)->h,
        .w = BASE_GAUGE(self->compass)->w,
        .h = BASE_GAUGE(self->compass)->h,
    };

    text_loc = (SDL_Rect){
        .x = (int)SDLExt_RectMidX(&compass_loc) - (BASE_GAUGE(self->caption)->w-1)/2,
        .y = 0,
        .w = BASE_GAUGE(self->caption)->w,
        .h = BASE_GAUGE(self->caption)->h
    };

    snprintf(value, 5, "%03d", (int)ANIMATED_GAUGE(self->compass)->animation.current);
    text_gauge_set_value(self->caption, value);

    base_gauge_render(BASE_GAUGE(self->caption), dt, destination, &rect_offset(&text_loc, location));
    base_gauge_render(BASE_GAUGE(self->compass), dt, destination, &rect_offset(&compass_loc, location));
}

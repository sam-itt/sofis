#include <stdlib.h>

#include "base-gauge.h"
#include "base-widget.h"
#include "button.h"
#include "softkey.h"
#include "text-gauge.h"


static BaseWidgetOps softkey_ops = { 0 };


static inline void softkey_draw_border(Softkey *self, Uint32 dt, RenderContext *ctx);
static void softkey_render(Softkey *self, Uint32 dt, RenderContext *ctx);

Softkey *softkey_new(const char *caption, FontResource font_id, int w, int h)
{
    Softkey *self;

    self = calloc(1, sizeof(Softkey));
    if(self){
        if(!softkey_init(self, caption, font_id, w, h)){
            return base_gauge_free(BASE_GAUGE(self));
        }
    }
    return self;
}

Softkey *softkey_init(Softkey *self, const char *caption, FontResource font_id, int w, int h)
{
    Button *super = button_init(BUTTON(self),
        caption, font_id,
        SDL_WHITE, (SDL_Color){32, 32, 32, SDL_ALPHA_OPAQUE},
        w, h
    );
    if(!super) return NULL;

    if(!softkey_ops.super.render){
        memcpy(&softkey_ops, BUTTON(self)->super.super.ops, sizeof(BaseWidgetOps));
        BASE_GAUGE_OPS(&softkey_ops)->render = (RenderFunc)softkey_render;
    }
    BASE_GAUGE(self)->ops = BASE_GAUGE_OPS(&softkey_ops);

    /*Offsets the internal TextGauge to account for (uneven) borders*/
    base_gauge_move_by(BASE_GAUGE(BUTTON(self)->text), 1, 2);

    return self;
}


void softkey_set_state(Softkey *self, SoftkeyState state)
{
    if(self->state == state) return;

    self->state = state;
    switch(self->state){
        case SOFTKEY_STATE_DISABLED:
            button_set_color(BUTTON(self), (SDL_Color){64, 64, 64, SDL_ALPHA_OPAQUE}, TEXT_COLOR);
            button_set_color(BUTTON(self), (SDL_Color){32, 32, 32, SDL_ALPHA_OPAQUE}, BACKGROUND_COLOR);
            break;
        case SOFTKEY_STATE_PRESSED:
            button_set_color(BUTTON(self), (SDL_Color){32, 32, 32, SDL_ALPHA_OPAQUE}, TEXT_COLOR);
            button_set_color(BUTTON(self), (SDL_Color){128, 128, 128, SDL_ALPHA_OPAQUE}, BACKGROUND_COLOR);
            break;
        case SOFTKEY_STATE_RELEASED:
            button_set_color(BUTTON(self), SDL_WHITE, TEXT_COLOR);
            button_set_color(BUTTON(self), (SDL_Color){32, 32, 32, SDL_ALPHA_OPAQUE}, BACKGROUND_COLOR);
            break;
    }
}


static void softkey_render(Softkey *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_fill(BASE_GAUGE(self), ctx, NULL, &(BUTTON(self)->text->bg_color), false);
    softkey_draw_border(self, dt, ctx);
}

static inline void softkey_draw_border(Softkey *self, Uint32 dt, RenderContext *ctx)
{
    /*TODO: Add the first top white line*/
    SDL_Rect top = (SDL_Rect){
        .x = 0,
        .y = 0,
        .w = base_gauge_w(BASE_GAUGE(self)),
        .h = 1
    };
    SDL_Rect top_second = (SDL_Rect){
        .x = 0,
        .y = 1,
        .w = base_gauge_w(BASE_GAUGE(self)),
        .h = 1
    };

    SDL_Rect left = (SDL_Rect){
        .x = 0,
        .y = 1,
        .w = 1,
        .h = base_gauge_h(BASE_GAUGE(self)) - 1 /*TODO: Fix potential overdrawing, i.e a gauge can draw out of its area*/
    };
    SDL_Rect right = (SDL_Rect){
        .x = base_gauge_w(BASE_GAUGE(self)) - 1,
        .y = 1,
        .w = 1,
        .h = base_gauge_h(BASE_GAUGE(self)) - 1
    };


    if(self->state == SOFTKEY_STATE_RELEASED){
        base_gauge_fill(BASE_GAUGE(self), ctx, &top, &SDL_WHITE, false);
        base_gauge_fill(BASE_GAUGE(self), ctx, &top_second, &SDL_LIGHTGREY, false);
        base_gauge_fill(BASE_GAUGE(self), ctx, &left, &SDL_LIGHTGREY, false);
        base_gauge_fill(BASE_GAUGE(self), ctx, &right, &SDL_BLACK, false);
    }else if(self->state == SOFTKEY_STATE_PRESSED){
        SDL_Rect pre_left = left;
        left.x++;
        base_gauge_fill(BASE_GAUGE(self), ctx, &top, &SDL_GREY, false);
        base_gauge_fill(BASE_GAUGE(self), ctx, &top_second, &SDL_BLACK, false);
        base_gauge_fill(BASE_GAUGE(self), ctx, &pre_left, &SDL_GREY, false);
        base_gauge_fill(BASE_GAUGE(self), ctx, &left, &SDL_BLACK, false);
        base_gauge_fill(BASE_GAUGE(self), ctx, &right, &SDL_WHITE, false);
   }
}

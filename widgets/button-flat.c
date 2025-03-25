#include <stdlib.h>
#include <assert.h>

#include "SDL_pcf.h"
#include "base-gauge.h"
#include "button-flat.h"
#include "misc.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "text-gauge.h"

static void button_flat_render(ButtonFlat *self, Uint32 dt, RenderContext *ctx);
static bool button_flat_handle_event(ButtonFlat *self, SDL_KeyboardEvent *event);

static BaseWidgetOps button_flat_ops = {
    .super.update_state = (StateUpdateFunc)NULL,
    .super.render = (RenderFunc)button_flat_render,
    .super.dispose = (DisposeFunc)NULL,
    .handle_event = (EventHandlerFunc)button_flat_handle_event
};


ButtonFlat *button_flat_new(const char *caption, FontResource font_id, SDL_Color text_color, SDL_Color bg_color, int w, int h)
{
    ButtonFlat *self;

    self = calloc(1, sizeof(ButtonFlat));
    if(self){
        if(!button_flat_init(self, caption, font_id, text_color, bg_color, w, h, 1)){
            return base_gauge_free(BASE_GAUGE(self));
        }
    }
    return self;
}

ButtonFlat *button_flat_init(ButtonFlat *self, const char *caption, FontResource font_id, SDL_Color text_color, SDL_Color bg_color, int w, int h, uintf8_t border_width)
{
    base_widget_init(BASE_WIDGET(self),
        &button_flat_ops,
        w, h
    );

    self->font_id = font_id;
    self->border_width = border_width;

    /*TODO: Have a text_gauge_get_size_request ?*/
    PCF_StaticFont *sfont = resource_manager_get_static_font(
        self->font_id, &text_color,
        1, caption
    );
    if(!sfont) return NULL;

    SDL_Rect text_rect;
    PCF_StaticFontGetSizeRequestRect(sfont, caption, &text_rect);

    if(text_rect.w > w - 2*self->border_width - 1 || text_rect.h > h - 2*self->border_width - 1){
        /*TODO: Have TextGauge render write an ellipsis(...) as a single char
         * where there is not enough space to write the full text*/
        /*TODO: Log the failure ?*/
        printf("ButtonFlat: Cannot init widget with a size of (w:%d,h:%d) while the minimum computed size is (w:%d,h:%d)\n",
            w, h,
            text_rect.w + 2*self->border_width + 1,
            text_rect.h + 2*self->border_width + 1
        );
        return NULL;
    }
    SDLExt_RectCenter(&text_rect, &(SDL_Rect){.w = w, .h = h});

    self->text = text_gauge_new(NULL, false, text_rect.w, text_rect.h);
    text_gauge_set_color(self->text, text_color, TEXT_COLOR);
    text_gauge_set_color(self->text, bg_color, BACKGROUND_COLOR);
    self->text->alignment = HALIGN_CENTER | VALIGN_MIDDLE;
    button_flat_set_caption(self, caption);

    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->text), text_rect.x , text_rect.y);

    return self;
}

bool button_flat_set_caption(ButtonFlat *self, const char *caption)
{
    bool rv;

    PCF_StaticFont *sfont = resource_manager_get_static_font(
        self->font_id, &self->text->text_color,
        1, caption
    );
    if(!sfont)
        return false;

    rv = text_gauge_set_value(self->text, caption);
    if(!rv)
        return rv;

    text_gauge_set_static_font(self->text, sfont);

    return true;
}

bool button_flat_set_color(ButtonFlat *self, SDL_Color color, Uint8 which)
{
    if(which == TEXT_COLOR){
        PCF_StaticFont *sfont = resource_manager_get_static_font(
            self->font_id, &color,
            1, self->text->value
        );
        if(!sfont)
            return false;

        text_gauge_set_static_font(self->text, sfont);
    }else if(which == BACKGROUND_COLOR){
        text_gauge_set_color(BUTTON_FLAT(self)->text, color, BACKGROUND_COLOR);
    }
    BASE_GAUGE(self)->dirty = true;
    return true;
}

static void button_flat_render(ButtonFlat *self, Uint32 dt, RenderContext *ctx)
{
//    base_gauge_render(BASE_GAUGE(self), dt, ctx);
    base_gauge_fill(BASE_GAUGE(self), ctx, NULL, &(self->text->bg_color), false);
    base_widget_draw_outline(BASE_WIDGET(self), ctx);
}



/*Should be button_flat_handle_sdl_event */
static bool button_flat_handle_event(ButtonFlat *self, SDL_KeyboardEvent *event)
{
    if(event->state != SDL_PRESSED) return true;

    switch(event->keysym.sym){
        case SDLK_RETURN:
            if(self->validated.callback)
                self->validated.callback(self->validated.target, BASE_WIDGET(self));
            else
                return false;
            break;
        case SDLK_UP: /*Fallthrough*/
        case SDLK_DOWN: /*Give back focus*/
            return false;
            break;
    }
    return true;
}



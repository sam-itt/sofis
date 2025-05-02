#include "softkey-bar.h"
#include "SDL_events.h"
#include "SDL_keycode.h"
#include "SDL_pcf.h"
#include "base-gauge.h"
#include "button.h"
#include "softkey.h"
#include "misc.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "softkey-model.h"

#define SOFTKEY_BAR_W 640
#define SOFTKEY_BAR_H 16
#define SOFTKEY_BUTTON_W 53
#define SOFTKEY_BUTTON_H 16


static intf8_t softkey_bar_key_to_button_index(SDL_KeyboardEvent *event);

static void softkey_bar_render(SoftkeyBar *self, Uint32 dt, RenderContext *ctx);
static bool softkey_bar_handle_event(SoftkeyBar *self, SDL_KeyboardEvent *event);

static BaseWidgetOps softkey_bar_ops = {
    .super.update_state = (StateUpdateFunc)NULL,
    .super.render = (RenderFunc)softkey_bar_render,
    .super.dispose = (DisposeFunc)NULL,
    .handle_event = (EventHandlerFunc)softkey_bar_handle_event
};


SoftkeyBar *softkey_bar_new(FontResource font_id, SoftkeyModel *model)
{
    SoftkeyBar *self;

    self = calloc(1, sizeof(SoftkeyBar));
    if(self){
        if(!softkey_bar_init(self, model, font_id, SOFTKEY_BAR_W, SOFTKEY_BAR_H)){
            return base_gauge_free(BASE_GAUGE(self));
        }
    }
    return self;
}

SoftkeyBar *softkey_bar_init(SoftkeyBar *self, SoftkeyModel *model, FontResource font_id, int w, int h)
{
    base_widget_init(BASE_WIDGET(self),
        &softkey_bar_ops,
        w, h
    );

    self->model = model;

    /* The following is a trick to pre-load needed glyph
     * TODO: Have a self-configuration mechanism for the ResourceManager:
     * save to a file what was needed in the previous run and merge as one
     * big font.
     * */
    PCF_StaticFont *sfont = resource_manager_get_static_font(
        font_id, &SDL_WHITE,
        1, PCF_UPPER_CASE
    );
    if(!sfont) return NULL;

    for(int i = 0, x = 2; i < N_SOFTKEYS; i++, x += SOFTKEY_BUTTON_W){
        SoftkeyDetails *details = self->model
                                ? softkey_model_get_details_at(self->model, i)
                                : NULL;
        softkey_init(&(self->buttons[i]),
            details ? details->caption : "",
            font_id,
            SOFTKEY_BUTTON_W, SOFTKEY_BUTTON_H
        );
        base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(&(self->buttons[i])), x , 0);
    }

    return self;
}

void softkey_bar_set_model(SoftkeyBar *self, SoftkeyModel *model)
{
    self->model = model;

    for(int i = 0, x = 2; i < N_SOFTKEYS; i++, x += SOFTKEY_BUTTON_W){
        SoftkeyDetails *details = self->model
                                ? softkey_model_get_details_at(self->model, i)
                                : NULL;
        button_set_caption(BUTTON(&(self->buttons[i])), details ? details->caption : "");
    }
    BASE_GAUGE(self)->dirty = true; /*TODO: Check if that is needed*/
}

void softkey_bar_push_model(SoftkeyBar *self, SoftkeyModel *model)
{
    model->prev = self->model;
    softkey_bar_set_model(self, model);
}

bool softkey_bar_pop_model(SoftkeyBar *self)
{
    SoftkeyModel *previous;

    if(!self->model) return false;

    previous = self->model->prev;
    self->model->prev = NULL;
    softkey_bar_set_model(self, previous);

    return true;
}

static void softkey_bar_render(SoftkeyBar *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_fill(BASE_GAUGE(self), ctx, NULL, &(SDL_BLACK), false);
}

/*Always return false, we do not care about focus*/
static bool softkey_bar_handle_event(SoftkeyBar *self, SDL_KeyboardEvent *event)
{
    if(!self->model) return false;
    if(event->state != SDL_PRESSED && event->state != SDL_RELEASED) return false;

    intf8_t idx = softkey_bar_key_to_button_index(event);
    if(idx < 0) return false;

    SoftkeyDetails *details = softkey_model_get_details_at(self->model, idx);
    if(!details) return false;

    if(event->state == SDL_PRESSED){
        softkey_set_state(&(self->buttons[idx]), SOFTKEY_STATE_PRESSED);
//        printf("%s: Pressed\n",__FUNCTION__);
        return true;
    }

    softkey_set_state(&(self->buttons[idx]), SOFTKEY_STATE_RELEASED);
//    printf("%s: Released\n",__FUNCTION__);
    /*TODO: Animate the button press, change state before doing the actions*/
    if(details->type == SOFTKEY_TYPE_REGULAR){
        //invoke callback
    }else if(details->type == SOFTKEY_TYPE_SEGUE){
        softkey_bar_push_model(self, details->action.next);
    }else if(details->type == SOFTKEY_TYPE_BACK){
        softkey_bar_pop_model(self);
    }

    return false;
}

static intf8_t softkey_bar_key_to_button_index(SDL_KeyboardEvent *event)
{
    switch(event->keysym.sym){
        case SDLK_F1: return 1;
        case SDLK_F2: return 2;
        case SDLK_F3: return 3;
        case SDLK_F4: return 4;
        case SDLK_F5: return 5;
        case SDLK_F6: return 6;
        case SDLK_F7: return 7;
        case SDLK_F8: return 8;
        case SDLK_F9: return 9;
        case SDLK_F10: return 10;
        case SDLK_F11: return 11;
        case SDLK_F12: return 12;
    }
    return -1;
}

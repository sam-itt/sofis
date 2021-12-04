#include <stdlib.h>
#include <string.h>

#include "SDL_pcf.h"
#include "button.h"
#include "base-gauge.h"
#include "resource-manager.h"
#include "sdl-colors.h"

static void button_update(Button *self);
static void button_render(Button *self, Uint32 dt, RenderContext *ctx);
static Button *button_dispose(Button *self);
static bool button_handle_event(Button *self, SDL_KeyboardEvent *event);

static BaseWidgetOps button_ops ={
    .super.update_state = (StateUpdateFunc)button_update,
    .super.render = (RenderFunc)button_render,
    .super.dispose = (DisposeFunc)button_dispose,
    .handle_event = (EventHandlerFunc)button_handle_event
};


Button *button_new(const char *caption, FontResource font_id, int w, int h)
{
    Button *self;

    self = calloc(1, sizeof(Button));
    if(self){
        if(!button_init(self, caption, font_id, w, h)){
            return base_gauge_free(BASE_GAUGE(self));
        }
    }
    return self;
}

Button *button_init(Button *self, const char *caption, FontResource font_id, int w, int h)
{
    base_widget_init(BASE_WIDGET(self),
        &button_ops,
        w, h
    );

    PCF_Font *font;
    font = resource_manager_get_font(font_id);
    self->sfont = resource_manager_get_static_font(font_id, &SDL_WHITE, 1, caption);
    if(!self->sfont)
        return NULL;

    self->caption = strdup(caption);
    if(!self->caption)
        return NULL;
    self->clen = strlen(self->caption);

    self->state.patches = calloc(self->clen, sizeof(PCF_StaticFontPatch));
    if(!self->state.patches)
        return NULL;

    return self;
}

static Button *button_dispose(Button *self)
{
    if(self->caption)
        free(self->caption);
    if(self->sfont)
        PCF_StaticFontUnref(self->sfont);
    if(self->state.patches)
        free(self->state.patches);

    return self;
}

static void button_update(Button *self)
{
    ButtonState *state;
    SDL_Rect cursor;

    state = &(self->state);

    SDL_Rect tarea = BASE_GAUGE(self)->frame;
    tarea.x = 0;
    tarea.y = 0;
    PCF_StaticFontGetSizeRequestRect(self->sfont, self->caption, &cursor);
    SDLExt_RectAlign(&cursor, &tarea, self->alignment);

    state->npatches = PCF_StaticFontPreWriteString(self->sfont,
        self->clen,
        self->caption,
        &cursor,
        self->clen,
        state->patches);
}

static void button_render(Button *self, Uint32 dt, RenderContext *ctx)
{
    for(int i = 0; i < self->state.npatches; i++){
        base_gauge_draw_static_font_patch(BASE_GAUGE(self),
            ctx,
            self->sfont,
            &self->state.patches[i]
        );
    }

    base_widget_draw_outline(BASE_WIDGET(self), ctx);
}

static bool button_handle_event(Button *self, SDL_KeyboardEvent *event)
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



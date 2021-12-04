#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "SDL_pcf.h"
#include "base-gauge.h"
#include "base-widget.h"
#include "resource-manager.h"
#include "sdl-colors.h"

#include "text-box.h"
#define TEXT_INCREMENT 10

static void kill_duplicates(char *base, size_t len);

static bool text_box_validate_font(TextBox *self);

static void text_box_render(TextBox *self, Uint32 dt, RenderContext *ctx);
static void text_box_update(TextBox *self);
static TextBox *text_box_dispose(TextBox *self);
static bool text_box_handle_event(TextBox *self, SDL_KeyboardEvent *event);

static BaseWidgetOps text_box_ops ={
    .super.render = (RenderFunc)text_box_render,
    .super.update_state = (StateUpdateFunc)text_box_update,
    .super.dispose = (DisposeFunc)text_box_dispose,
    .handle_event = (EventHandlerFunc)text_box_handle_event
};

TextBox *text_box_new(FontResource font_id, int width, int height)
{
    TextBox *self;

    self = calloc(1, sizeof(TextBox));
    if(self){
        if(!text_box_init(self, font_id, width, height))
            return base_gauge_free(BASE_GAUGE(self));
    }
    return self;
}

TextBox *text_box_init(TextBox *self, FontResource font_id, int width, int height)
{
    base_widget_init(BASE_WIDGET(self),
        &text_box_ops,
        width, height
    );

    self->alen = TEXT_INCREMENT;
    self->text = calloc(self->alen, sizeof(char));
    if(!self->text)
        return NULL;
    self->text[0] = 32;
    self->tlen = 2;

    self->font_id = font_id;
    self->alen = TEXT_INCREMENT;

    PCF_Font *font;
    font = resource_manager_get_font(self->font_id);
    size_t nchars = ceilf((BASE_GAUGE(self)->frame.w*1.0)/PCF_FontCharWidth(font));

    /* We need one more when scrolled: a 4-chars wide textbox can display
     * 1 char partly, 3 chars and 1 more char partly with first width + last width
     * = 1 full char width*/
    self->state.apatches = nchars+1;
    self->state.patches = malloc(sizeof(PCF_StaticFontRectPatch)*self->state.apatches);
    BASE_GAUGE(self)->dirty = true;
    return self;
}

static TextBox *text_box_dispose(TextBox *self)
{
    if(self->text)
        free(self->text);
    if(self->allowed_chars)
        free(self->allowed_chars);
    if(self->sfont)
        PCF_StaticFontUnref(self->sfont);
    if(self->state.patches)
        free(self->state.patches);
    return self;
}

bool text_box_set_allowed_chars(TextBox *self, bool keep_order, int nsets, ...)
{
    char *tmp;
    size_t tlen;
    va_list ap;

    tlen = 0;
    va_start(ap, nsets);
    for(int i = 0; i < nsets; i++){
        tmp = va_arg(ap, char*);
        tlen += strlen(tmp);
    }
    va_end(ap);

    char *iter;
    self->nallowed_chars = tlen;
    self->allowed_chars = calloc(self->nallowed_chars + 1, sizeof(char));

    iter = self->allowed_chars;
    va_start(ap, nsets);
    for(int i = 0; i < nsets; i++){
        tmp = va_arg(ap, char*);
        strcpy(iter, tmp);
        iter += strlen(tmp);
    }

    if(!keep_order){
        qsort(self->allowed_chars,
            self->nallowed_chars, sizeof(char),
            (__compar_fn_t) strcmp
        );
        filter_dedup(self->allowed_chars, self->nallowed_chars);
    }else{
        kill_duplicates(self->allowed_chars, self->nallowed_chars);
    }

    return text_box_validate_font(self);
}

/**
 * @param text should not have leading or trailing spaces
 *
 */
bool text_box_set_text(TextBox *self, const char *text)
{
    int ntlen;

    ntlen = strlen(text);
    if((ntlen+1) >= self->alen){
        size_t nalen;
        char *tmp;
        nalen = ceilf((ntlen+1)*1.0f/TEXT_INCREMENT);
        nalen *= TEXT_INCREMENT;
        tmp = realloc(self->text, nalen);
        if(!tmp)
            return false;
        self->text = tmp;
        self->alen = nalen;
    }
    strncpy(self->text, text, ntlen+1);
    self->tlen = ntlen+1;

    if(!text_box_validate_font(self))
        return false;
    PCF_StaticFontGetSizeRequestRect(self->sfont, self->text, &(self->text_size));

    return true;
}


bool text_box_move_cursor(TextBox *self, int_fast8_t direction)
{
    if(direction > 0){
        /* 0: a char, 1: \0, tlen=2
         * As tlen includes that last NULL byte, the last char of the string is
         * NOT tlen-1, but tlen-2*/
        if(self->current_index == self->tlen-2){ /*We are on the last char*/
            if(self->current_index >= self->alen-2){
                return false; /*TODO: Grow*/
            }

            if(self->text[self->current_index] == ' '){
                if(self->current_index == 0)
                    return false; /*Can't move left if first char is space*/
                if(self->text[self->current_index-1] == ' ')
                    return false; /*Can't move left if previous char is also space*/
            }

            self->current_index++;
            self->text[self->current_index] = self->current_index > 0
                                            ? self->text[self->current_index-1]
                                            : 32;
            self->text[self->current_index+1] = '\0';
            self->tlen++;
            PCF_StaticFontGetSizeRequestRect(self->sfont, self->text, &(self->text_size));
        }else{
            self->current_index++;
        }
    }else if(direction < 0 && self->current_index > 0){
        if(self->text[self->current_index] == ' '){
            while(self->current_index > 0 && self->text[self->current_index] == ' '){
                self->text[self->current_index] = '\0';
                self->current_index--;
                self->tlen--;
            }
        }else{
            self->current_index--;
        }
    }else{
        return false;
    }

    BASE_GAUGE(self)->dirty = true;
    return true;
}

void text_box_cycle_char(TextBox *self, int_fast8_t direction)
{
    if(self->allowed_chars){
        size_t ac_index; /*index in the allowed char set*/
        char *ac_ptr;

        ac_index = 0;
        if(self->text[self->current_index] != '\0'){
            ac_ptr = strchr(self->allowed_chars, self->text[self->current_index]);
            if(ac_ptr)
                ac_index = (size_t)(ac_ptr - self->allowed_chars);
        }

        /*TODO: Have a utility function that makes a value go round*/
        if(ac_index == 0 && direction < 0)
            ac_index = self->nallowed_chars-1;
        if(ac_index == (self->nallowed_chars-1) && direction > 0)
            ac_index = 0;

        ac_index += direction;

        self->text[self->current_index] = self->allowed_chars[ac_index];
    }else{ //ASCII
        self->text[self->current_index] += direction; /*will loop over*/
        if(self->text[self->current_index] >= 127) /*TODO: Modulo?*/
            self->text[self->current_index] = 32;
    }
    BASE_GAUGE(self)->dirty = true;

    if(self->changed_callback)
        self->changed_callback(self, self->userdata);
}

bool text_box_release_focus(TextBox *self)
{
    BASE_GAUGE(self)->dirty = true;
    return false;
}

static void text_box_update(TextBox *self)
{
    TextBoxState *state;

    state = &(self->state);
    int charx = self->current_index * PCF_StaticFontCharWidth(self->sfont);
    /*no need to -1 *both* for cmp*/
    int outx = charx + PCF_StaticFontCharWidth(self->sfont); /*first pixel out of glyph*/

    if(outx > state->startx + BASE_GAUGE(self)->frame.w){
        state->startx = (outx-1) - (BASE_GAUGE(self)->frame.w-1);
    }else if(charx < state->startx){
        state->startx = charx;
    }

    size_t ibegin; /*string indices*/
    ibegin = state->startx / PCF_StaticFontCharWidth(self->sfont);
    state->first_index = ibegin;

    SDL_Rect tarea = BASE_GAUGE(self)->frame;
    state->npatches = PCF_StaticFontPreWriteStringOffset(
        self->sfont,
        self->tlen-1,
        self->text+ibegin,
        &tarea,
        state->startx * -1, 0,
        state->apatches, state->patches
    );

    BASE_GAUGE(self)->dirty = false;
}


static void text_box_render(TextBox *self, Uint32 dt, RenderContext *ctx)
{
    for(int i = 0; i < self->state.npatches; i++){
        if(self->state.first_index + i == self->current_index && BASE_WIDGET(self)->has_focus){
            /*TODO: PCF_StaticFontPatch(Src|Dst)Rect */
            base_gauge_fill(BASE_GAUGE(self), ctx, &(SDL_Rect){
                    self->state.patches[i].dst.x,
                    self->state.patches[i].dst.y,
                    self->state.patches[i].src.w,
                    self->state.patches[i].src.h
                }, &SDL_RED, false
            );
        }
        if(self->state.patches[i].src.x < 0) continue;

        base_gauge_draw_static_font_rect_patch(BASE_GAUGE(self), ctx,
            self->sfont,
            &self->state.patches[i]
        );
    }

    base_widget_draw_outline(BASE_WIDGET(self), ctx);
}


static bool text_box_handle_event(TextBox *self, SDL_KeyboardEvent *event)
{
    if(event->state != SDL_PRESSED) return true;

    switch(event->keysym.sym){
        case SDLK_UP:
            text_box_cycle_char(self, -1);
            break;
        case SDLK_DOWN:
            text_box_cycle_char(self, +1);
            break;
        case SDLK_LEFT:
            text_box_move_cursor(self, -1);
            break;
        case SDLK_RIGHT:
            text_box_move_cursor(self, +1);
            break;
        case SDLK_RETURN:
            return text_box_release_focus(self);
            break;
    }
    return true;
}

static bool text_box_validate_font(TextBox *self)
{
    if(self->sfont){
        if(PCF_StaticFontCanWrite(self->sfont, NULL, self->text)){
            if(self->allowed_chars){
                if(PCF_StaticFontCanWrite(self->sfont, NULL, self->allowed_chars))
                    return true;
            }else{
                return true;
            }

        }
        /*We get here only if we have a font and that font can't write self->text*/
        PCF_StaticFontUnref(self->sfont);
    }

    if(self->allowed_chars){
        self->sfont = resource_manager_get_static_font(self->font_id,
            &SDL_WHITE,
            2,
            self->text,
            self->allowed_chars
        );
    }else{
        self->sfont = resource_manager_get_static_font(self->font_id,
            &SDL_WHITE,
            1,
            self->text
        );
    }
    if(!self->sfont)
        return false;

    PCF_StaticFontCreateTexture(self->sfont);
    return self->sfont->texture != NULL;
}

/*
 * Remove duplicates characters from @p base, while keeping the order.
 * When duplicates are found, only the first occurence is kept.
 *
 * This function is slower than filter_dedup but doesn't need sorting
 * and thus can preserve original ordering.
 *
 * @param base The string to filter
 * @param len THe string len, -1 to have the function compute it.
 */
static void kill_duplicates(char *base, size_t len)
{
    len = len > -1 ? len : strlen(base);

    for(int i = 0; i < len; i++){
        for(int j = i+1; j < len; j++){
            if(base[j] == base[i])
                base[j] = '\0';
        }
    }

    for(int i = 0; i < len; i++){
        if(base[i] == '\0'){
            /*look for next real char*/
            int next;
            for(next = i; next < len && base[next] == '\0'; next++);
            for(int j = next; j < len; j++)
                base[i+(j-next)] = base[j];
        }
    }
}

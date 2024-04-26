/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL_keycode.h"
#include "SDL_pcf.h"
#include "base-gauge.h"
#include "list-model.h"
#include "resource-manager.h"
#include "sdl-colors.h"

#include "misc.h"
#include "text-box.h"
#include "list-box.h"


static void list_box_update(ListBox *self, Uint32 dt);
static void list_box_render(ListBox *self, Uint32 dt, RenderContext *ctx);
static ListBox *list_box_dispose(ListBox *self);
static bool list_box_handle_event(ListBox *self, SDL_KeyboardEvent *event);
static BaseWidgetOps list_box_ops = {
    .super.render = (RenderFunc)list_box_render,
    .super.update_state = (StateUpdateFunc)list_box_update,
    .super.dispose = (DisposeFunc)list_box_dispose,
    .handle_event = (EventHandlerFunc)list_box_handle_event,
};


static inline void list_box_fire_selection_changed(ListBox *self);

/**
 * Takes ownership of the model (i.e. will free it)
 *
 */
ListBox *list_box_new(FontResource font_id, int width, int height)
{
    ListBox *self;

    self = calloc(1, sizeof(ListBox));
    if(self){
        if(!list_box_init(self, font_id, width, height))
            return base_gauge_free(BASE_GAUGE(self));
    }
    return self;
}

ListBox *list_box_init(ListBox *self, FontResource font_id, int width, int height)
{
    base_widget_init(BASE_WIDGET(self),
        &list_box_ops,
        width, height
    );


    font_id = font_id;
    PCF_Font *font = resource_manager_get_font(font_id);
    size_t nhchars = ceilf((BASE_GAUGE(self)->frame.w*1.0)/PCF_FontCharWidth(font));
    size_t nvchars = ceilf((BASE_GAUGE(self)->frame.h*1.0)/PCF_FontCharHeight(font));

    /* We need one more when scrolled horizontally and one more when scrolled
     * vertically.
     * A 4-chars wide/tall listbox can display
     * 1 char partly, 3 chars and 1 more char partly with
     * first width + last width = 1 full char width*/
    self->state.apatches = (nhchars+1) * (nvchars+1);
    self->state.patches = malloc(sizeof(PCF_StaticFontRectPatch)
            * self->state.apatches
        );
        printf("allocated %zu patches\n", self->state.apatches);

        self->sfont = resource_manager_get_static_font(font_id,
            &SDL_WHITE,
            1,
            ASCII_PRINTABLE
        );
        if(!self->sfont)
            return NULL;
        PCF_StaticFontCreateTexture(self->sfont);


    BASE_GAUGE(self)->dirty = true;

    return self;
}

static ListBox *list_box_dispose(ListBox *self)
{
    if(self->sfont)
        PCF_StaticFontUnref(self->sfont);
    if(self->state.patches)
        free(self->state.patches);
    if(self->model)
        list_model_free(self->model);
    return self;
}

void list_box_set_model(ListBox *self, ListModel *model)
{
    self->model = model;
    model->listbox = self;
    list_box_model_changed(self);
}

void list_box_model_changed(ListBox *self)
{
    self->text_size.h = self->model->nrows * PCF_StaticFontCharHeight(self->sfont);
    self->text_size.w = self->model->maxlen * PCF_StaticFontCharWidth(self->sfont);

    self->selected_row = 0;
    self->state.selected_h = 0;
    self->state.selected_y = 0;
    BASE_GAUGE(self)->dirty = true;

    list_box_fire_selection_changed(self);
}

bool list_box_horizontal_scroll(ListBox *self, int_fast8_t direction)
{
    if(direction < 0 && self->state.offset.x <= 0)
        return false;
    if(direction > 0 && self->state.offset.x+BASE_GAUGE(self)->frame.w >= self->text_size.w)
        return false;
    self->state.offset.x += direction;

    BASE_GAUGE(self)->dirty = true;
    return true;
}

bool list_box_vertical_scroll(ListBox *self, int_fast8_t direction)
{
    size_t old_selected;

    old_selected = self->selected_row;
    if(direction > 0){
        if(self->selected_row < self->model->nrows-1)
            self->selected_row++;
    }else if(direction < 0){
        if(self->selected_row > 0)
            self->selected_row--;
    }
    if(self->selected_row != old_selected){
        list_box_fire_selection_changed(self);
        BASE_GAUGE(self)->dirty = true;
    }

    return BASE_GAUGE(self)->dirty;
}


static void list_box_update(ListBox *self, Uint32 dt)
{
    ListBoxState *state;
    SDL_Rect glyph;
    size_t current_height;

    state = &(self->state);
    if(self->model->nrows == 0){
        state->npatches = 0;
        return;
    }

    int chary = self->selected_row * PCF_StaticFontCharHeight(self->sfont);
    /*no need to -1 *both* for cmp*/
    int outy = chary + PCF_StaticFontCharHeight(self->sfont); /*first pixel out of glyph*/
    if(outy > state->offset.y + BASE_GAUGE(self)->frame.h){
        state->offset.y = (outy-1) - (BASE_GAUGE(self)->frame.h-1);
    }else if(chary < state->offset.y){
        state->offset.y = chary;
    }

    size_t ibegin_v, iend_v; /*string indices*/
    ibegin_v = state->offset.y / PCF_StaticFontCharHeight(self->sfont);
    Uint32 endy = state->offset.y + (MIN(BASE_GAUGE(self)->frame.h, self->text_size.h)-1);
    iend_v = ceilf(endy*1.0f/PCF_StaticFontCharHeight(self->sfont));
    iend_v = MIN(iend_v, self->model->nrows);

    state->npatches = 0; /*TODO compute*/
    current_height = 0;
    for(int y = ibegin_v; y < iend_v; y++){
        int yremaining = BASE_GAUGE(self)->frame.h - current_height - 1;
        int line_height = MIN(
            PCF_StaticFontCharHeight(self->sfont),
            yremaining
        );
        SDL_Rect tarea = {
            0,0,
            BASE_GAUGE(self)->frame.w,
            BASE_GAUGE(self)->frame.h
        };
        tarea.y += current_height;
        tarea.h = (BASE_GAUGE(self)->frame.h-1) - tarea.y + 1;

        int lyoffset = 0;
        if(y == ibegin_v){
            lyoffset = state->offset.y % PCF_StaticFontCharHeight(self->sfont);
        }

        size_t n_line_patches;
        n_line_patches = PCF_StaticFontPreWriteStringOffset(
            self->sfont,
            self->model->row_lenghts[y],
            self->model->rows[y].label,
            &tarea,
            state->offset.x * -1, lyoffset * -1,
            state->apatches - state->npatches,
            state->patches + state->npatches
        );
#if 0
        printf("Line %d patches:\n", y);
        for(int i = state->npatches; i < state->npatches + n_line_patches; i++){
            printf("\tPatch[%d]: src(x:%d y:%d w:%d h:%d) dst:(x:%d y:%d)\n", i,
                state->patches[i].src.x, state->patches[i].src.y,
                state->patches[i].src.w, state->patches[i].src.h,
                state->patches[i].dst.x,
                state->patches[i].dst.y
            );
        }
#endif
        state->npatches += n_line_patches;
        if(n_line_patches){
            line_height = state->patches[state->npatches-1].src.h;
        }
        if(y == self->selected_row && n_line_patches){
            state->selected_y = state->patches[state->npatches-1].dst.y;
            state->selected_h = line_height;
        }
        current_height += line_height;
    }
    BASE_GAUGE(self)->dirty = false;
}

static void list_box_render(ListBox *self, Uint32 dt, RenderContext *ctx)
{
    SDL_Color cursor_background;

    cursor_background = (SDL_Color){112,128,144, SDL_ALPHA_OPAQUE};
    if(BASE_WIDGET(self)->has_focus)
        cursor_background = (SDL_Color){255,0,0,SDL_ALPHA_OPAQUE};


    if(self->state.selected_h){
        base_gauge_fill(BASE_GAUGE(self), ctx, &(SDL_Rect){
                0, self->state.selected_y,
                BASE_GAUGE(self)->frame.w,
                self->state.selected_h
            },
            &cursor_background, false
        );

    }

    for(int i = 0; i < self->state.npatches; i++){
        if(self->state.patches[i].src.x < 0) continue;

        base_gauge_draw_static_font_rect_patch(BASE_GAUGE(self), ctx,
            self->sfont,
            &self->state.patches[i]
        );
    }

    base_widget_draw_outline(BASE_WIDGET(self), ctx);
}

static bool list_box_handle_event(ListBox *self, SDL_KeyboardEvent *event)
{
    if(event->state != SDL_PRESSED) return true;

    switch(event->keysym.sym){
        case SDLK_UP:
            list_box_vertical_scroll(self, -1);
            break;
        case SDLK_DOWN:
            list_box_vertical_scroll(self, 1);
            break;
        case SDLK_LEFT:
            list_box_horizontal_scroll(self, -1);
            break;
        case SDLK_RIGHT:
            list_box_horizontal_scroll(self, 1);
            break;
        case SDLK_RETURN:
            return false;
            break;
    }
    return true;
}

static inline void list_box_fire_selection_changed(ListBox *self)
{
    if(self->selection_changed.callback){
        self->selection_changed.callback(
            self->selection_changed.target,
            BASE_WIDGET(self)
        );
    }
}

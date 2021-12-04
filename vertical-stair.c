/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_image.h>
#include "SDL_pcf.h"

#include "base-gauge.h"
#include "generic-layer.h"
#include "vertical-stair.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "misc.h"

static void vertical_stair_render(VerticalStair *self, Uint32 dt, RenderContext *ctx);
static void vertical_stair_update_state(VerticalStair *self, Uint32 dt);
static void *vertical_stair_dispose(VerticalStair *self);
static BaseGaugeOps vertical_stair_ops = {
   .render = (RenderFunc)vertical_stair_render,
   .update_state = (StateUpdateFunc)vertical_stair_update_state,
   .dispose = (DisposeFunc)vertical_stair_dispose
};


VerticalStair *vertical_stair_new(const char *bg_img, const char *cursor_img, PCF_StaticFont *font)
{
    VerticalStair *self;

    self = calloc(1, sizeof(VerticalStair));
    if(self){
        if(!vertical_stair_init(self, bg_img, cursor_img, font)){
            return base_gauge_dispose(BASE_GAUGE(self));
        }
    }
    return self;
}

VerticalStair *vertical_stair_init(VerticalStair *self, const char *bg_img, const char *cursor_img, PCF_StaticFont *font)
{
    bool rv;

    rv = generic_layer_init_from_file(GENERIC_LAYER(&self->scale), bg_img);
    if(!rv)
        return NULL;
    self->scale.ppv = 43.0/1000.0; /*0.0426*/
    self->scale.start = -2279;
    self->scale.end = 2279;

    rv = generic_layer_init_from_file(&self->cursor, cursor_img);
    self->font = font;
    if(!rv || !self->font)
        return NULL; //TODO: Will leak self->scale.ruler
    PCF_StaticFontRef(self->font);
    generic_layer_build_texture(GENERIC_LAYER(&self->scale));
    generic_layer_build_texture(&self->cursor);

    base_gauge_init(BASE_GAUGE(self),
        &vertical_stair_ops,
        generic_layer_w(&self->cursor),
        generic_layer_h(GENERIC_LAYER(&self->scale))
    );

    return self;
}

static void *vertical_stair_dispose(VerticalStair *self)
{
    vertical_strip_dispose(&self->scale);
    generic_layer_dispose(&self->cursor);
    if(self->font)
        PCF_FreeStaticFont(self->font);
    return self;
}

bool vertical_stair_set_value(VerticalStair *self, float value, bool animated)
{
    return sfv_gauge_set_value(SFV_GAUGE(self), value, animated);
}


static void vertical_stair_update_state(VerticalStair *self, Uint32 dt)
{
    float y;
    char number[VS_VALUE_MAX_LEN];
    int ivalue;
    int len;

//    printf("%s %p value: %f\n", __FUNCTION__, self, SFV_GAUGE(self)->value);
    ivalue = round(SFV_GAUGE(self)->value);
    len = snprintf(number, VS_VALUE_MAX_LEN, "% -d", ivalue);
    vertical_strip_clip_value(&self->scale, &SFV_GAUGE(self)->value);

    y = vertical_strip_resolve_value(&self->scale, SFV_GAUGE(self)->value, true);
    y = round(round(y) - generic_layer_h(&self->cursor)/2.0);
    self->state.cloc = (SDL_Rect){
        1, y,
        generic_layer_w(&self->cursor),
        generic_layer_h(&self->cursor)
    };

    PCF_StaticFontGetSizeRequestRect(self->font, number, &self->state.tloc);
    SDLExt_RectAlign(&self->state.tloc,
        &self->state.cloc,
        HALIGN_LEFT | VALIGN_MIDDLE
    );
    self->state.tloc.x += self->font->metrics.characterWidth;

    SDL_Rect glyph, cursor;
    cursor = self->state.tloc;
    self->state.nchars = PCF_StaticFontPreWriteString(self->font,
        len, number,
        &cursor,
        VS_VALUE_MAX_LEN-1, self->state.chars
    );
}

static void vertical_stair_render(VerticalStair *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        GENERIC_LAYER(&self->scale), NULL,
        &(SDL_Rect){
            0, 0,
            generic_layer_w(GENERIC_LAYER(&self->scale)),
            generic_layer_h(GENERIC_LAYER(&self->scale))
        }
    );

    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        &self->cursor, NULL, &self->state.cloc
    );

    for(int i = 0; i < self->state.nchars; i++){
        base_gauge_draw_static_font_glyph(BASE_GAUGE(self),
            ctx,
            self->font,
            &self->state.chars[i].src,
            &self->state.chars[i].dst
        );
    }
}

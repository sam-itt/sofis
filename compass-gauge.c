/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include "base-gauge.h"
#include "compass-gauge.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "misc.h"
#include "text-gauge.h"
#include "res-dirs.h"

static void compass_gauge_render(CompassGauge *self, Uint32 dt, RenderContext *ctx);
static void compass_gauge_update_state(CompassGauge *self, Uint32 dt);
static void *compass_gauge_dispose(CompassGauge *self);
static BaseGaugeOps compass_gauge_ops = {
   .render = (RenderFunc)compass_gauge_render,
   .update_state = (StateUpdateFunc)compass_gauge_update_state,
   .dispose = (DisposeFunc)compass_gauge_dispose
};


CompassGauge *compass_gauge_new(void)
{
    CompassGauge *self;
    self = calloc(1, sizeof(CompassGauge));
    if(self){
        if(!compass_gauge_init(self)){
            return base_gauge_dispose(BASE_GAUGE(self));
        }
    }
    return(self);
}

CompassGauge *compass_gauge_init(CompassGauge *self)
{
    bool rv;

    rv = generic_layer_init_from_file(&self->outer, IMG_DIR"/compass-outer.png");
    if(!rv){
        printf("Couldn't load compass outer ring\n");
        return NULL;
    }
    rv = generic_layer_init_from_file(&self->inner, IMG_DIR"/compass-inner.png");
    if(!rv){
        printf("Couldn't load compass inner ring\n");
        return NULL;
    }

    self->caption = text_gauge_new("000°", true, 28, 12);
    if(!self->caption)
        return NULL;
    text_gauge_set_color(self->caption, SDL_BLACK, BACKGROUND_COLOR);
    self->caption->alignment = HALIGN_CENTER | VALIGN_MIDDLE;
    text_gauge_set_static_font(self->caption,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, PCF_DIGITS, "°"
        )
    );

    base_gauge_init(BASE_GAUGE(self),
        &compass_gauge_ops,
        generic_layer_w(&self->outer),
        generic_layer_h(&self->outer) + base_gauge_h(BASE_GAUGE(self->caption))
    );

    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->caption),
        (int)SDLExt_RectMidX(&BASE_GAUGE(self)->frame)
        - (int)SDLExt_RectMidX(&BASE_GAUGE(self->caption)->frame),
        0
    );

    self->icenter = (SDL_Point){
        .x = (generic_layer_w(&self->inner) - 1)/2,
        .y = (generic_layer_h(&self->inner) - 1)/2,
    };

    self->outer_rect = (SDL_Rect){
        .x = 0,
        .y = base_gauge_h(BASE_GAUGE(self->caption)),
        .w = generic_layer_w(&self->outer),
        .h = generic_layer_h(&self->outer)
    };

    self->inner_rect = (SDL_Rect){
        .x = (generic_layer_w(&self->outer) - 1)/2 - self->icenter.x,
        .y = (generic_layer_h(&self->outer) - 1)/2 - self->icenter.y + self->outer_rect.y,
        .w = generic_layer_w(&self->inner),
        .h = generic_layer_h(&self->inner)
    };

    generic_layer_build_texture(&self->outer);
    generic_layer_build_texture(&self->inner);

#if !USE_SDL_GPU
	self->state.rbuffer = SDL_CreateRGBSurfaceWithFormat(0,
        generic_layer_w(&self->inner),
        generic_layer_h(&self->inner),
        32, SDL_PIXELFORMAT_RGBA32
    );
	self->renderer =  SDL_CreateSoftwareRenderer(self->state.rbuffer);
    self->texture = SDL_CreateTextureFromSurface(self->renderer, self->inner.canvas);
#endif
    return self;
}

static void *compass_gauge_dispose(CompassGauge *self)
{
    generic_layer_dispose(&self->outer);
    generic_layer_dispose(&self->inner);

    return self;
}

bool compass_gauge_set_value(CompassGauge *self, float value, bool animated)
{
    bool rv = true;
    BaseAnimation *animation;

    value = fmod(value, 360.0);
    if(value < 0)
        value += 360.0;
    /*Temp fix for 'goes backwards effet when going from a value
     * near 360 to a value after 360
     * */
    animated = false;
    return sfv_gauge_set_value(SFV_GAUGE(self), value, animated);
}

static void compass_gauge_update_state(CompassGauge *self, Uint32 dt)
{
#if !USE_SDL_GPU
    SDL_FillRect(self->state.rbuffer, NULL, 0x00000000);
	if(SFV_GAUGE(self)->value != 0){
        SDL_RenderCopyEx(self->renderer, self->texture,
            NULL, NULL,
            SFV_GAUGE(self)->value * -1.0f,
            &self->icenter, SDL_FLIP_NONE
        );
	}else{
        SDL_BlitSurface(self->inner.canvas, NULL,self->state.rbuffer, NULL);
    }
#endif
    text_gauge_set_value_formatn(self->caption,
        4, /*3 digits plus degree sign*/
        "%03d", (int)SFV_GAUGE(self)->value
    );
}

static void compass_gauge_render(CompassGauge *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        &self->outer,
        NULL, &self->outer_rect);
#if USE_SDL_GPU
    base_gauge_blit_rotated_texture(BASE_GAUGE(self), ctx,
        self->inner.texture,
        NULL,
        SFV_GAUGE(self)->value * -1.0f,
        &self->icenter,
        &self->inner_rect,
        NULL);
#else
    base_gauge_blit(BASE_GAUGE(self), ctx, self->state.rbuffer, NULL, &self->inner_rect);
#endif
}


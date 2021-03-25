/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "roll-slip-gauge.h"
#include "misc.h"
#include "res-dirs.h"

#define sign(x) (((x) > 0) - ((x) < 0))

static void roll_slip_gauge_render(RollSlipGauge *self, Uint32 dt, RenderContext *ctx);
static void roll_slip_gauge_update_state(RollSlipGauge *self, Uint32 dt);
static RollSlipGauge *roll_slip_gauge_dispose(RollSlipGauge *self);
static BaseGaugeOps roll_slip_gauge_ops = {
   .render = (RenderFunc)roll_slip_gauge_render,
   .update_state = (StateUpdateFunc)roll_slip_gauge_update_state,
   .dispose = (DisposeFunc)roll_slip_gauge_dispose
};


RollSlipGauge *roll_slip_gauge_new(void)
{
	RollSlipGauge *self;

	self = calloc(1, sizeof(RollSlipGauge));
	if(self){
		if(!roll_slip_gauge_init(self)){
            return base_gauge_free(BASE_GAUGE(self));
		}
	}
	return self;
}

RollSlipGauge *roll_slip_gauge_init(RollSlipGauge *self)
{
    base_gauge_init(BASE_GAUGE(self), &roll_slip_gauge_ops, 183, 183);

    generic_layer_init_from_file(&self->arc, IMG_DIR"/roll-arc.png");
    if(!self->arc.canvas) return NULL;
    generic_layer_build_texture(&self->arc);

    generic_layer_init_from_file(&self->marker, IMG_DIR"/roll-marker.png");
    if(!self->marker.canvas) return NULL;
    generic_layer_build_texture(&self->marker);

    generic_layer_init_from_file(&self->slip_marker, IMG_DIR"/slip-marker.png");
    if(!self->slip_marker.canvas) return NULL;
    generic_layer_build_texture(&self->slip_marker);


    //TODO: Center on with surfaces, generic_layer_midX, base_gauge_midx
//	self->marker_rect.x = (base_gauge_w(BASE_GAUGE(self))/2) - tmp->w/2; //TODO Middle function that handle even/odd
//    printf("self->marker_rect.x: %d\n",self->marker_rect.x);
    self->marker_rect.x = 91 - 4;
	self->marker_rect.y = 11;
	self->marker_rect.w = generic_layer_w(&self->marker);
	self->marker_rect.h = generic_layer_h(&self->marker);

    self->state.slip_rect.x = SDLExt_RectMidX(&self->marker_rect) - generic_layer_w(&self->slip_marker)/2;
    self->state.slip_rect.y = SDLExt_RectLastY(&self->marker_rect) + 2;
    self->state.slip_rect.w = generic_layer_w(&self->slip_marker);
    self->state.slip_rect.h = generic_layer_h(&self->slip_marker);

#if !USE_SDL_GPU
    self->state.rbuffer = SDL_CreateRGBSurfaceWithFormat(0,
        base_gauge_w(BASE_GAUGE(self)),
        base_gauge_h(BASE_GAUGE(self)),
        32, SDL_PIXELFORMAT_RGBA32
    );
    self->renderer = SDL_CreateSoftwareRenderer(self->state.rbuffer);
    self->arc_texture = SDL_CreateTextureFromSurface(self->renderer, tmp);
#endif

	return self;
}

static RollSlipGauge *roll_slip_gauge_dispose(RollSlipGauge *self)
{
    generic_layer_dispose(&self->arc);
    generic_layer_dispose(&self->marker);
    generic_layer_dispose(&self->slip_marker);
#if !USE_SDL_GPU
    if(self->state.rbuffer)
        SDL_FreeSurface(self->state.rbuffer);
    SDL_DestroyRenderer(self->renderer); /*Will also free self->arrow*/
#endif

    return self;
}

bool roll_slip_gauge_set_value(RollSlipGauge *self, float value, bool animated)
{
    bool rv = true;
    BaseAnimation *animation;
    animated = false;
	if(value > 60.0 || value < -60.0)
		value = sign(value)*65;

    return sfv_gauge_set_value(SFV_GAUGE(self), value, animated);
}

bool roll_slip_gauge_set_slip(RollSlipGauge *self, float value, bool animated)
{
    bool rv = true;
    BaseAnimation *animation;
    animated = false;

    if(fabsf(value) > 90.0)
        return false;

    self->slip = value;
    BASE_GAUGE(self)->dirty = true;

    return true;
}


static void roll_slip_gauge_update_state(RollSlipGauge *self, Uint32 dt)
{
    float ratio = self->slip/90.0;

    int base_x = SDLExt_RectMidX(&self->marker_rect) - generic_layer_w(&self->slip_marker)/2;
    int increment = roundf(ratio * (self->marker_rect.w));

    self->state.slip_rect.x = base_x + increment;
#if !USE_SDL_GPU
    SDL_FillRect(self->state.rbuffer, NULL, 0x00000000);
	SDL_RenderCopyEx(self->renderer,
        self->arc_texture,
        NULL, /*Whole texture*/
        NULL, /*Whole surface*/
        -SFV_GAUGE(self)->value,
        NULL, /*Rotate on center*/
        SDL_FLIP_NONE);
#endif
}

static void roll_slip_gauge_render(RollSlipGauge *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        &self->marker, NULL,
        &self->marker_rect
    );

    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        &self->slip_marker, NULL,
        &self->state.slip_rect
    );

#if USE_SDL_GPU
    base_gauge_blit_rotated_texture(BASE_GAUGE(self), ctx,
    self->arc.texture,
    NULL,
    -SFV_GAUGE(self)->value,
    NULL, /*rotate on center*/
    NULL, /*Whole gauge*/
    NULL);
#else
    base_gauge_blit(BASE_GAUGE(self), ctx, self->state.rbuffer, NULL, NULL);
#endif
}

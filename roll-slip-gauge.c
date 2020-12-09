#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "SDL_surface.h"
#include "base-gauge.h"
#include "generic-layer.h"
#include "roll-slip-gauge.h"
#include "sdl-colors.h"

#define sign(x) (((x) > 0) - ((x) < 0))

static void roll_slip_gauge_render(RollSlipGauge *self, Uint32 dt, RenderContext *ctx);
static void roll_slip_gauge_update_state(RollSlipGauge *self, Uint32 dt);
static BaseGaugeOps roll_slip_gauge_ops = {
   .render = (RenderFunc)roll_slip_gauge_render,
   .update_state = (StateUpdateFunc)roll_slip_gauge_update_state
};


RollSlipGauge *roll_slip_gauge_new(void)
{
	RollSlipGauge *self;

	self = calloc(1, sizeof(RollSlipGauge));
	if(self){
		if(!roll_slip_gauge_init(self)){
			free(self);
			return NULL;
		}
	}
	return self;
}

RollSlipGauge *roll_slip_gauge_init(RollSlipGauge *self)
{
    base_gauge_init(BASE_GAUGE(self), &roll_slip_gauge_ops, 175, 184);

    generic_layer_init_from_file(&self->arc, "roll-indicator.png");
    if(!self->arc.canvas) return NULL;
    generic_layer_build_texture(&self->arc);

	SDL_Surface *tmp = IMG_Load("arrow-needleless.png");
    if(!tmp) return NULL; //TODO: Can leak self->arc
#if USE_SDL_GPU
    self->arrow = GPU_CopyImageFromSurface(tmp);
#else
    self->state.rbuffer = SDL_CreateRGBSurfaceWithFormat(0,
        base_gauge_w(BASE_GAUGE(self)),
        base_gauge_h(BASE_GAUGE(self)),
        32, SDL_PIXELFORMAT_RGBA32
    );
    self->renderer = SDL_CreateSoftwareRenderer(self->state.rbuffer);
    self->arrow = SDL_CreateTextureFromSurface(self->renderer, tmp);
#endif
	SDL_FreeSurface(tmp);

	//Arc 0°: 86/10
//	rect.x = 86 - round(self->arrow->w/2.0);
	self->state.arrow_rect.x = 87 - 6;
	self->state.arrow_rect.y = 10;
	self->state.arrow_rect.h = 103;
	self->state.arrow_rect.w = 13;

	self->state.arrow_center.x = self->state.arrow_rect.x;
	self->state.arrow_center.y = self->state.arrow_rect.y - 94;

	self->state.arrow_center.x = self->state.arrow_rect.w/2;
	self->state.arrow_center.y = 90; /*Radius 94.5 or 92.725*/


	return self;
}

void roll_slip_gauge_dispose(RollSlipGauge *self)
{
    base_gauge_dispose(BASE_GAUGE(self));
    generic_layer_dispose(&self->arc);
#if !USE_SDL_GPU
    if(self->state.rbuffer)
        SDL_FreeSurface(self->state.rbuffer);
    SDL_DestroyRenderer(self->renderer); /*Will also free self->arrow*/
#endif
}

void roll_slip_gauge_free(RollSlipGauge *self)
{
	roll_slip_gauge_dispose(self);
	free(self);
}

bool roll_slip_gauge_set_value(RollSlipGauge *self, float value, bool animated)
{
    bool rv = true;
    BaseAnimation *animation;

	if(value > 60.0 || value < -60.0)
		value = sign(value)*65;

    if(animated){
        if(BASE_GAUGE(self)->nanimations == 0){
            animation = base_animation_new(TYPE_FLOAT, 1, &self->value);
            base_gauge_add_animation(BASE_GAUGE(self), animation);
            base_animation_unref(animation);/*base_gauge takes ownership*/
        }else{
            animation = BASE_GAUGE(self)->animations[0];
        }
        base_animation_start(animation, self->value, value, DEFAULT_DURATION);
    }else{
        self->value = value;
        BASE_GAUGE(self)->dirty = true;
    }

    return rv;
}

static void roll_slip_gauge_update_state(RollSlipGauge *self, Uint32 dt)
{
#if !USE_SDL_GPU
	SDL_RenderCopyEx(self->renderer,
        self->arrow, NULL,
        &self->state.arrow_rect,
        value,
        &self->state.arrow_center,
        SDL_FLIP_NONE);
#endif
}

static void roll_slip_gauge_render(RollSlipGauge *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        &self->arc, NULL,
        &(SDL_Rect){
            0,0,
            generic_layer_w(&self->arc),
            generic_layer_h(&self->arc),
        }
    );
#if USE_SDL_GPU
    base_gauge_blit_rotated_texture(BASE_GAUGE(self), ctx,
        self->arrow, NULL,
        self->value,
        &self->state.arrow_center,
        &self->state.arrow_rect,
        NULL
    );
#else
    base_gauge_blit(BASE_GAUGE(self), ctx, self->state.rbuffer, NULL, &self->state.arrow_rect);
#endif
}

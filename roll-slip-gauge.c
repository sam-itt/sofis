#include "SDL_image.h"
#include <stdio.h>
#include <stdlib.h>

#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "animated-gauge.h"
#include "buffered-gauge.h"
#include "roll-slip-gauge.h"
#include "sdl-colors.h"


#define sign(x) (((x) > 0) - ((x) < 0))

static void roll_slip_gauge_render_value(RollSlipGauge *self, float value);
static AnimatedGaugeOps roll_slip_gauge_ops = {
   .render_value = (ValueRenderFunc)roll_slip_gauge_render_value
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
    animated_gauge_init(ANIMATED_GAUGE(self), ANIMATED_GAUGE_OPS(&roll_slip_gauge_ops), 175, 184);
    BUFFERED_GAUGE(self)->max_ops = 2;

	self->arc = IMG_Load("roll-indicator.png");
    if(!self->arc) return NULL;

	SDL_Surface *tmp = IMG_Load("arrow-needleless.png");
    if(!tmp) return NULL; //TODO: Can leak self->arc
#if USE_SDL_RENDERER
	self->arrow = SDL_CreateTextureFromSurface(g_renderer, tmp);
    self->tarc = SDL_CreateTextureFromSurface(g_renderer, self->arc);
#else
	self->renderer = SDL_CreateSoftwareRenderer(buffered_gauge_get_view(BUFFERED_GAUGE(self)));
	self->arrow = SDL_CreateTextureFromSurface(self->renderer, tmp);
#endif
	SDL_FreeSurface(tmp);

	return self;
}

void roll_slip_gauge_dispose(RollSlipGauge *self)
{
    animated_gauge_dispose(ANIMATED_GAUGE(self));
    SDL_FreeSurface(self->arc);
#if !USE_SDL_RENDERER
    SDL_DestroyRenderer(self->renderer); /*Will also free self->arrow*/
#endif
}

void roll_slip_gauge_free(RollSlipGauge *self)
{
	roll_slip_gauge_dispose(self);
	free(self);
}

static void roll_slip_gauge_render_value(RollSlipGauge *self, float value)
{
	SDL_Rect rect;
	SDL_Point center;

	if(value > 60.0 || value < -60.0)
		value = sign(value)*65;

	value *= -1.0;
#if USE_SDL_RENDERER
    buffered_gauge_blit_texture(BUFFERED_GAUGE(self), self->tarc, NULL, &(SDL_Rect){0,0,self->arc->w,self->arc->h});
#else
    buffered_gauge_fill(BUFFERED_GAUGE(self), NULL, &SDL_TRANSPARENT);
    buffered_gauge_blit(BUFFERED_GAUGE(self), self->arc, NULL, NULL);
#endif
	//Arc 0°: 86/10
//	rect.x = 86 - round(self->arrow->w/2.0);
	rect.x = 87 - 6;
	rect.y = 10;
	rect.h = 103;
	rect.w = 13;

	center.x = rect.x;
	center.y = rect.y - 94;

	center.x = rect.w/2;
	center.y = 90; /*Radius 94.5 or 92.725*/
#if USE_SDL_RENDERER
    buffered_gauge_blit_rotated_texture(BUFFERED_GAUGE(self), self->arrow, NULL, value, &center, &rect, NULL);
#else
	SDL_RenderCopyEx(self->renderer, self->arrow, NULL,&rect, value, &center, SDL_FLIP_NONE);
#endif
}

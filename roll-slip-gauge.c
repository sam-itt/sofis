#include "SDL_image.h"
#include <stdio.h>
#include <stdlib.h>

#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "animated-gauge.h"
#include "roll-slip-gauge.h"
#include "sdl-colors.h"


#define sign(x) (((x) > 0) - ((x) < 0))

static void roll_slip_gauge_render_value(RollSlipGauge *self, float value);
static void roll_slip_gauge_render_value_to(RollSlipGauge *self, float value, SDL_Surface *destination, SDL_Rect *location);


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
	self->parent.view = SDL_CreateRGBSurfaceWithFormat(0, 175, 184, 32, SDL_PIXELFORMAT_RGBA32);
    self->parent.w = 175;
    self->parent.h = 184;
	self->parent.renderer = (ValueRenderFunc)roll_slip_gauge_render_value;
	self->parent.renderer_to = (ValueRenderToFunc)roll_slip_gauge_render_value_to;
	self->parent.damaged = true;

	self->arc = IMG_Load("roll-indicator.png");
	SDL_Surface *tmp = IMG_Load("arrow-needleless.png");

	self->renderer = SDL_CreateSoftwareRenderer(self->parent.view);
	self->arrow = SDL_CreateTextureFromSurface(self->renderer, tmp);
	SDL_FreeSurface(tmp);

	return self;
}

void roll_slip_gauge_dispose(RollSlipGauge *self)
{
    animated_gauge_dispose(ANIMATED_GAUGE(self));
    SDL_FreeSurface(self->arc);
    SDL_DestroyRenderer(self->renderer); /*Will also free self->arrow*/
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

	SDL_FillRect(self->parent.view, NULL, SDL_MapRGBA(self->parent.view->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT));
	SDL_BlitSurface(self->arc,NULL, self->parent.view, NULL);

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

	SDL_RenderCopyEx(self->renderer, self->arrow, NULL,&rect, value, &center, SDL_FLIP_NONE);
}

static void roll_slip_gauge_render_value_to(RollSlipGauge *self, float value, SDL_Surface *destination, SDL_Rect *location)
{
	SDL_Rect rect;
	SDL_Point center;

	if(value > 60.0 || value < -60.0)
		value = sign(value)*65;

	value *= -1.0;

    /*Clear the area before drawing. TODO, move upper in animated_gauge ?*/
//    SDL_FillRect(destination, &(SDL_Rect){location->x,location->y,ANIMATED_GAUGE(self)->w,ANIMATED_GAUGE(self)->h}, SDL_UFBLUE(destination));
//    SDL_FillRect(destination, &(SDL_Rect){location->x,location->y,ANIMATED_GAUGE(self)->w,ANIMATED_GAUGE(self)->h}, SDL_UTRANSPARENT(destination));

	SDL_BlitSurface(self->arc,NULL, destination, location);

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

    /*We are using parent view as a rotation buffer only. Consider using a local buffer when deactivating parent view*/
	SDL_FillRect(self->parent.view, NULL, SDL_MapRGBA(self->parent.view->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT));
	SDL_RenderCopyEx(self->renderer, self->arrow, NULL,&rect, value, &center, SDL_FLIP_NONE);

	SDL_BlitSurface(self->parent.view, NULL, destination, location);

}

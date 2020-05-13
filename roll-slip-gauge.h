#include "SDL_surface.h"
#ifndef ROLL_SLIP_GAUGE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "animated-gauge.h"

typedef struct{
	AnimatedGauge super;

	SDL_Surface *arc;
	SDL_Texture *arrow;

#if USE_SDL_RENDERER
    SDL_Texture *tarc;
#else
	SDL_Renderer *renderer;
#endif
}RollSlipGauge;


RollSlipGauge *roll_slip_gauge_new(void);
RollSlipGauge *roll_slip_gauge_init(RollSlipGauge *self);
void roll_slip_gauge_dispose(RollSlipGauge *self);
void roll_slip_gauge_free(RollSlipGauge *self);

#define ROLL_SLIP_GAUGE_H
#endif /* ROLL_SLIP_GAUGE_H */

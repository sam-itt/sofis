#ifndef ROLL_SLIP_GAUGE_H
#define ROLL_SLIP_GAUGE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "animated-gauge.h"

typedef struct{
	AnimatedGauge super;

	SDL_Surface *arc;

#if USE_SDL_GPU
	GPU_Image *arrow;
    GPU_Image *tarc;
#else
    SDL_Texture *arrow;
	SDL_Renderer *renderer;
#endif
}RollSlipGauge;


RollSlipGauge *roll_slip_gauge_new(void);
RollSlipGauge *roll_slip_gauge_init(RollSlipGauge *self);
void roll_slip_gauge_dispose(RollSlipGauge *self);
void roll_slip_gauge_free(RollSlipGauge *self);

#endif /* ROLL_SLIP_GAUGE_H */

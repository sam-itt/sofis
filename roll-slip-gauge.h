#ifndef ROLL_SLIP_GAUGE_H
#define ROLL_SLIP_GAUGE_H

#include <SDL2/SDL.h>

#include "sfv-gauge.h"
#include "generic-layer.h"

typedef struct{
    SDL_Rect arrow_rect;
    SDL_Point arrow_center;
#if !USE_SDL_GPU
    SDL_Surface *rbuffer; /*rotation buffer*/
#endif
}RollSlipGaugeState;

typedef struct{
	SfvGauge super;

    GenericLayer arc;

#if USE_SDL_GPU
	GPU_Image *arrow;
#else
    SDL_Texture *arrow;
	SDL_Renderer *renderer;
#endif
    RollSlipGaugeState state;
}RollSlipGauge;


RollSlipGauge *roll_slip_gauge_new(void);
RollSlipGauge *roll_slip_gauge_init(RollSlipGauge *self);
void roll_slip_gauge_dispose(RollSlipGauge *self);
void roll_slip_gauge_free(RollSlipGauge *self);

bool roll_slip_gauge_set_value(RollSlipGauge *self, float value, bool animated);
#endif /* ROLL_SLIP_GAUGE_H */

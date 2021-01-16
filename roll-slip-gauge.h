#ifndef ROLL_SLIP_GAUGE_H
#define ROLL_SLIP_GAUGE_H

#include <SDL2/SDL.h>

#include "sfv-gauge.h"
#include "generic-layer.h"

typedef struct{
    SDL_Rect slip_rect;
#if !USE_SDL_GPU
    SDL_Surface *rbuffer; /*rotation buffer*/
#endif
}RollSlipGaugeState;

typedef struct{
	SfvGauge super;
    float slip;

    GenericLayer arc;
    GenericLayer marker;
    GenericLayer slip_marker;

#if !USE_SDL_GPU
    SDL_Texture *arc_texture;
	SDL_Renderer *renderer;
#endif
    SDL_Rect marker_rect;

    RollSlipGaugeState state;
}RollSlipGauge;


RollSlipGauge *roll_slip_gauge_new(void);
RollSlipGauge *roll_slip_gauge_init(RollSlipGauge *self);
RollSlipGauge *roll_slip_gauge_dispose(RollSlipGauge *self);
RollSlipGauge *roll_slip_gauge_free(RollSlipGauge *self);

bool roll_slip_gauge_set_value(RollSlipGauge *self, float value, bool animated);
bool roll_slip_gauge_set_slip(RollSlipGauge *self, float value, bool animated);
#endif /* ROLL_SLIP_GAUGE_H */

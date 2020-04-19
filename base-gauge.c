#include <stdio.h>
#include <stdlib.h>

#include "base-gauge.h"

BaseGauge *base_gauge_init(BaseGauge *self, BaseGaugeOps *ops, int w, int h)
{
    self->w = w;
    self->h = h;

    self->ops = ops;

    return self;
}

SDL_Surface *base_gauge_render(BaseGauge *self, Uint32 dt)
{
    return self->ops->render(self, dt);
}
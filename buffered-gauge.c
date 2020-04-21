#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL_rect.h"
#include "SDL_surface.h"
#include "base-gauge.h"
#include "buffered-gauge.h"
#include "sdl-colors.h"

/*BufferGauge implements BaseGauge::render and triggers the actual
rendering only if its view have been damaged.*/

void buffered_gauge_render(BufferedGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);
static BufferedGaugeOps buffered_gauge_ops = {
    .super = {
        .render = (RenderFunc)buffered_gauge_render
    },
   .render = NULL
};


BufferedGauge *buffered_gauge_init(BufferedGauge *self, BufferedGaugeOps *ops, int w, int h)
{
    ops->super = buffered_gauge_ops.super; /*Take care of the chain-up here so caller only needs to set .render*/
    base_gauge_init(BASE_GAUGE(self), BASE_GAUGE_OPS(ops), w, h);

    self->view = SDL_CreateRGBSurfaceWithFormat(0, BASE_GAUGE(self)->w,  BASE_GAUGE(self)->h, 32, SDL_PIXELFORMAT_RGBA32);
    if(!self->view)
        return NULL;
    SDL_SetColorKey(self->view, SDL_TRUE, SDL_UCKEY(self->view));
    self->damaged = true;

    return self;
}

void buffered_gauge_dispose(BufferedGauge *self)
{
    if(self->view)
        SDL_FreeSurface(self->view);
}

void buffered_gauge_render(BufferedGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location)
{
    BufferedGaugeOps *ops;

    if(self->damaged){
        ops = BUFFERED_GAUGE_OPS(BASE_GAUGE(self)->ops);
        ops->render(self, dt, destination, location);
        self->damaged = false;
    }
    SDL_BlitSurface(self->view, NULL, destination, location);
}

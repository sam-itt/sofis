#ifndef BUFFERED_GAUGE_H
#define BUFFERED_GAUGE_H
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "base-gauge.h"
#include "view.h"

typedef struct{
    BaseGaugeOps super;

    RenderFunc render;
}BufferedGaugeOps;

#define BUFFERED_GAUGE_OPS(ops) ((BufferedGaugeOps*)(ops))

typedef struct{
    BaseGauge super;

    SDL_Surface *view;
    bool damaged;
}BufferedGauge;

#define BUFFERED_GAUGE(self) ((BufferedGauge*)(self))

#define buffered_gauge_clear(self) view_clear((self)->view)

BufferedGauge *buffered_gauge_init(BufferedGauge *self, BufferedGaugeOps *ops, int w, int h);
void buffered_gauge_dispose(BufferedGauge *self);

void buffered_gauge_render(BufferedGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);
//BufferedGauge *buffered_gauge_init(BufferedGauge *self, BufferedGaugeOps *ops, int w, int h, BufferType type, ...);
//BufferedGauge *buffered_gauge_vainit(BufferedGauge *self, BufferedGaugeOps *ops, int w, int h, BufferType btype, va_list ap);

#endif /* BUFFERED_GAUGE_H */

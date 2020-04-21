#ifndef BUFFERED_GAUGE_H
#define BUFFERED_GAUGE_H
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "base-gauge.h"
#include "view.h"

typedef enum{
    BUFFER_OWN,
    BUFFER_SHARED
}BufferType;

typedef struct{
    BaseGaugeOps super;

    RenderFunc render;
}BufferedGaugeOps;

#define BUFFERED_GAUGE_OPS(ops) ((BufferedGaugeOps*)(ops))

typedef struct{
    BaseGauge super;

    BufferType type;
    SDL_Point offset;

    SDL_Surface *view;
    bool damaged;
}BufferedGauge;

#define BUFFERED_GAUGE(self) ((BufferedGauge*)(self))

#define buffered_gauge_clear(self) view_clear((self)->view)

#define buffered_gauge_get_view(self) ((self)->view ? (self)->view : buffer_gauge_build_view((self)))

BufferedGauge *buffered_gauge_init(BufferedGauge *self, BufferedGaugeOps *ops, int w, int h);
void buffered_gauge_dispose(BufferedGauge *self);

SDL_Surface *buffer_gauge_build_view(BufferedGauge *self);

void buffered_gauge_render(BufferedGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);

#endif /* BUFFERED_GAUGE_H */

#ifndef BASE_GAUGE_H
#define BASE_GAUGE_H

#include <SDL2/SDL.h>
#include <SDL_gpu.h>

typedef union{
    SDL_Surface *surface;
    GPU_Target* target;
}RenderTarget /*__attribute__ ((__transparent_union__))*/;

typedef void (*RenderFunc)(void *self, Uint32 dt, RenderTarget destination, SDL_Rect *location);

typedef struct{
    RenderFunc render;
}BaseGaugeOps;

typedef struct{
    BaseGaugeOps *ops;

    int w,h;
}BaseGauge;

#define BASE_GAUGE_OPS(self) ((BaseGaugeOps*)(self))
#define BASE_GAUGE(self) ((BaseGauge *)(self))

BaseGauge *base_gauge_init(BaseGauge *self, BaseGaugeOps *ops, int w, int h);

void base_gauge_render(BaseGauge *self, Uint32 dt, RenderTarget destination, SDL_Rect *location);
#endif /* BASE_GAUGE_H */

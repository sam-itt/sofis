#ifndef BASE_GAUGE_H
#define BASE_GAUGE_H

#include <SDL2/SDL.h>

typedef SDL_Surface* (*RenderFunc)(void *self, Uint32 dt);
typedef void (*RenderToFunc)(void *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);

typedef struct{
    RenderFunc render;
    RenderToFunc render_to;
}BaseGaugeOps;

typedef struct{
    BaseGaugeOps *ops;

    int w,h;
}BaseGauge;

#define BASE_GAUGE_OPS(self) ((BaseGaugeOps*)(self))
#define BASE_GAUGE(self) ((BaseGauge *)(self))

BaseGauge *base_gauge_init(BaseGauge *self, BaseGaugeOps *ops, int w, int h);

SDL_Surface *base_gauge_render(BaseGauge *self, Uint32 dt);
void base_gauge_render_to(BaseGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);

#endif /* BASE_GAUGE_H */

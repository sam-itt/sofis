#ifndef ANIMATED_GAUGE_H
#define ANIMATED_GAUGE_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "basic-animation.h"

typedef void (*ValueRenderFunc)(void *self, float value);
typedef void (*ValueRenderToFunc)(void *self, float value, SDL_Surface *destination, SDL_Rect *location);

typedef struct{
    SDL_Surface *view;
    int w,h;

    float value;
    BasicAnimation animation;
    bool damaged;

    ValueRenderFunc renderer;
    ValueRenderToFunc renderer_to;
}AnimatedGauge;

#define ANIMATED_GAUGE(self) ((AnimatedGauge *)(self))

#define animated_gauge_moving(self) ((self)->animation.current != (self)->value || (self)->damaged)
void animated_gauge_dispose(AnimatedGauge *self);

void animated_gauge_set_value(AnimatedGauge *self, float value);

SDL_Surface *animated_gauge_render(AnimatedGauge *self, Uint32 dt);
void animated_gauge_render_to(AnimatedGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);
#endif /* ANIMATED_GAUGE_H */

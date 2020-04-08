#ifndef ANIMATED_GAUGE_H
#define ANIMATED_GAUGE_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "basic-animation.h"

#define SDL_WHITE (SDL_Color){255, 255, 255, SDL_ALPHA_OPAQUE}
#define SDL_RED (SDL_Color){255, 0, 0, SDL_ALPHA_OPAQUE}
#define SDL_BLACK (SDL_Color){0, 0, 0, SDL_ALPHA_OPAQUE}


#define SDL_UBLACK(surface) (SDL_MapRGB((surface)->format, 0, 0,0))
#define SDL_UCKEY(surface) (SDL_MapRGB((surface)->format, 255, 0,255))

typedef void (*ValueRenderFunc)(void *self, float value);

typedef struct{
    SDL_Surface *view;
    float value;
    BasicAnimation animation;
    bool damaged;

    ValueRenderFunc renderer;
}AnimatedGauge;

#define ANIMATED_GAUGE(self) ((AnimatedGauge *)(self))

#define animated_gauge_moving(self) ((self)->animation.current != (self)->value || (self)->damaged)
void animated_gauge_dispose(AnimatedGauge *self);

void animated_gauge_set_value(AnimatedGauge *self, float value);
SDL_Surface *animated_gauge_render(AnimatedGauge *self, Uint32 dt);
#endif /* ANIMATED_GAUGE_H */

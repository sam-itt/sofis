#ifndef ANIMATED_GAUGE_H
#define ANIMATED_GAUGE_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "basic-animation.h"
#include "base-gauge.h"
#include "view.h"

typedef void (*ValueRenderFunc)(void *self, float value);
typedef void (*ValueRenderToFunc)(void *self, float value, SDL_Surface *destination, SDL_Rect *location);

typedef struct {
    BaseGaugeOps parent;
    ValueRenderFunc render_value;
    ValueRenderToFunc render_value_to;
}AnimatedGaugeOps;

typedef struct{
    BaseGauge parent;

    SDL_Surface *view;
    bool damaged;
    float value;
    BasicAnimation animation;
}AnimatedGauge;

#define ANIMATED_GAUGE(self) ((AnimatedGauge *)(self))
#define ANIMATED_GAUGE_OPS(self) ((AnimatedGaugeOps *)(self))

#define animated_gauge_moving(self) ((self)->animation.current != (self)->value || (self)->damaged)
#define animated_gauge_clear(self) view_clear((self)->view)


AnimatedGauge *animated_gauge_init(AnimatedGauge *self, AnimatedGaugeOps *ops, int w, int h);
void animated_gauge_dispose(AnimatedGauge *self);

void animated_gauge_set_value(AnimatedGauge *self, float value);
#endif /* ANIMATED_GAUGE_H */

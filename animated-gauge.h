#ifndef ANIMATED_GAUGE_H
#define ANIMATED_GAUGE_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "base-gauge.h"
#include "basic-animation.h"
#include "buffered-gauge.h"
#include "view.h"

typedef void (*ValueRenderFunc)(void *self, float value);

typedef struct {
    BufferedGaugeOps super;
    ValueRenderFunc render_value;
}AnimatedGaugeOps;

typedef struct{
    BufferedGauge super;

    float value;
    BasicAnimation animation;
}AnimatedGauge;

#define ANIMATED_GAUGE(self) ((AnimatedGauge *)(self))
#define ANIMATED_GAUGE_OPS(self) ((AnimatedGaugeOps *)(self))

#define animated_gauge_moving(self) ((self)->animation.current != (self)->value)


AnimatedGauge *animated_gauge_init(AnimatedGauge *self, AnimatedGaugeOps *ops, int w, int h);
void animated_gauge_dispose(AnimatedGauge *self);

void animated_gauge_set_value(AnimatedGauge *self, float value);
#endif /* ANIMATED_GAUGE_H */

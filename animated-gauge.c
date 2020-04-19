#include <stdio.h>
#include <stdlib.h>

#include "animated-gauge.h"
#include "base-gauge.h"
#include "sdl-colors.h"

#define SPIN_DURATION 1000 /*ms*/

static SDL_Surface *animated_gauge_render(AnimatedGauge *self, Uint32 dt);

static AnimatedGaugeOps animated_gauge_ops = {
    .parent = {
        .render = (RenderFunc)animated_gauge_render
    },
   .render_value = NULL
};


AnimatedGauge *animated_gauge_init(AnimatedGauge *self, AnimatedGaugeOps *ops, int w, int h)
{
    ops->parent = animated_gauge_ops.parent; /*Take care of the chain-up here so caller only needs to set .render_value*/

    base_gauge_init(BASE_GAUGE(self), BASE_GAUGE_OPS(ops), w, h);
    self->view = SDL_CreateRGBSurfaceWithFormat(0, BASE_GAUGE(self)->w,  BASE_GAUGE(self)->h, 32, SDL_PIXELFORMAT_RGBA32);
//    self->view = SDL_CreateRGBSurface(0, BASE_GAUGE(self)->w, BASE_GAUGE(self)->h, 32, 0, 0, 0, 0);
    if(!self->view)
        return NULL;
    SDL_SetColorKey(self->view, SDL_TRUE, SDL_UCKEY(self->view));
    self->damaged = true;
    return self;
}

void animated_gauge_dispose(AnimatedGauge *self)
{
    if(self->view)
        SDL_FreeSurface(self->view);
}


void animated_gauge_clear(AnimatedGauge *self)
{
    SDL_FillRect(self->view, NULL, SDL_UCKEY(self->view));
}

void animated_gauge_set_value(AnimatedGauge *self, float value)
{
    basic_animation_start(&self->animation, self->value, value, SPIN_DURATION);
    self->value = value;
}

/**
 * Gives a drawing representing the current gauge state. animated_gauge_opsReturn
 * value must not be freed by the caller.
 *
 * @param dt time elapsed since the previous call (miliseanimated_gauge_opsconds)
 * @return pointer to a SDL_Surface representing the curranimated_gauge_opsent gauge
 * state. Object-owned, do not free
 *
 */
static SDL_Surface *animated_gauge_render(AnimatedGauge *self, Uint32 dt)
{
    float _current;
    AnimatedGaugeOps *ops;

    if(animated_gauge_moving(self)){
        _current = basic_animation_loop(&self->animation, dt);

        ops = ANIMATED_GAUGE_OPS(BASE_GAUGE(self)->ops);
        ops->render_value(self, _current);
        self->damaged = false;
    }
    return self->view;
}

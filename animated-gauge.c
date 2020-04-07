#include <stdio.h>
#include <stdlib.h>

#include "animated-gauge.h"

#define SPIN_DURATION 1000 /*ms*/

void animated_gauge_dispose(AnimatedGauge *self)
{
    if(self->view)
        SDL_FreeSurface(self->view);
}

void animated_gauge_set_value(AnimatedGauge *self, float value)
{
    basic_animation_start(&self->animation, self->value, value, SPIN_DURATION);
    self->value = value;
}

/**
 * Gives a drawing representing the current gauge state. Return
 * value must not be freed by the caller.
 *
 * @param dt time elapsed since the previous call (miliseconds)
 * @return pointer to a SDL_Surface representing the current gauge
 * state. Object-owned, do not free
 *
 */
SDL_Surface *animated_gauge_render(AnimatedGauge *self, Uint32 dt)
{
    float _current;

    if(animated_gauge_moving(self)){
        _current = basic_animation_loop(&self->animation, dt);

        self->renderer(self, _current);
        self->damaged = false;
    }
    return self->view;
}

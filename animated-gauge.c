#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_surface.h"
#include "animated-gauge.h"
#include "base-gauge.h"
#include "buffered-gauge.h"
#include "sdl-colors.h"

#define SPIN_DURATION 1000 /*ms*/

/**
 * AnimatedGauge overrides BufferGauge's BaseGauge::render implementation to
 * provide its own (AnimatedGauge::base_renderer)  that damages the  buffer is
 * the animation is running, and then chains-up to BufferGauge's renderer,
 * which will in turn call AnimatedGauge::renderer which will ask the gauge to
 * draw the current animation value (Actual gauges only need to implement
 * render_value(self, float)).
 *
 * Might need to read that a couple of times.
 *
 * Summary:
 *
 * BaseGauge
 *  +exports render as BaseGaugeOps->render
 * BufferGauge (extends BaseGauge)
 *  +Implements BaseGauge::render as buffer_gauge_render
 *  +Exports (another) render as BufferGaugeOps->render
 * AnimatedGauge (extends BufferGauge)
 *  +Exports render_value as AnimatedGaugeOps->render_value
 *  +Implements BaseGauge::render as animated_gauge_base_render,
 *   that chains up to buffer_gauge_render
 *  +Implements BufferGauge::render as animated_gauge_render that will
 *   -Compute the animation value for current time
 *   -Call the gauge render_value for that value
 * RandomGauge (extends AnimatedGauge)
 *  +Implements AnimatedGauge::render_value as random_gauge_render_value(self, value)
 *
 *  Callgraphs:
 *
 *  Complete draw (damaged cache):
 *  base_gauge_render(random_gauge, ...)
 *   animated_gauge_base_render
 *    buffered_gauge_render
 *     animated_gauge_render
 *      random_gauge_render_value
 *
 * Call graph for a cached draw:
 * base_gauge_render(random_gauge, ...)
 *  animated_gauge_base_render
 *   buffered_gauge_render
 */

static void animated_gauge_base_render(AnimatedGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);
static void animated_gauge_render(AnimatedGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);
/*
static AnimatedGaugeOps animated_gauge_ops = {
    .super = {
        .render = (RenderFunc)animated_gauge_render
    },
   .render_value = NULL
};
*/
static AnimatedGaugeOps animated_gauge_ops = {
    .super = {
       .render = (RenderFunc)animated_gauge_render
    },
    .render_value = NULL
};



AnimatedGauge *animated_gauge_init(AnimatedGauge *self, AnimatedGaugeOps *ops, int w, int h)
{
    ops->super = animated_gauge_ops.super; /*Take care of the chain-up here so caller only needs to set .render_value*/
    buffered_gauge_init(BUFFERED_GAUGE(self), BUFFERED_GAUGE_OPS(ops), w, h);
    /* Override the BaseGauge::render function, we'll need to manually
     * chain-up to buffered_gauge_render
     * */
    BASE_GAUGE_OPS(ops)->render = (RenderFunc)animated_gauge_base_render;
    return self;
}

void animated_gauge_dispose(AnimatedGauge *self)
{
    buffered_gauge_dispose(BUFFERED_GAUGE(self));
}

void animated_gauge_set_value(AnimatedGauge *self, float value)
{
    basic_animation_start(&self->animation, self->value, value, SPIN_DURATION);
    self->value = value;
}

/**
 * Draws the gauge current state at the specified location
 *
 * @param dt time elapsed since the previous call (miliseanimated_gauge_opsconds)
 * @param destination the SDL_Surface to draw to
 * @param location offset in the surface
 *
 */
static void animated_gauge_render(AnimatedGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location)
{
    float _current;
    AnimatedGaugeOps *ops;

    _current = basic_animation_loop(&self->animation, dt);

    ops = ANIMATED_GAUGE_OPS(BASE_GAUGE(self)->ops);
    ops->render_value(self, _current);
    /*render_value works on the gauge->view so destination and location are
     * unused at this step. They are be used by the caller buffered_gauge_render*/
}

static void animated_gauge_base_render(AnimatedGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location)
{
   /*While moving, damage the superclass buffer & chain-up*/
    if(animated_gauge_moving(self))
        BUFFERED_GAUGE(self)->damaged = true;
    buffered_gauge_render(BUFFERED_GAUGE(self), dt, destination, location);
}

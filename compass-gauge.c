#include <stdio.h>
#include <stdlib.h>

#include "animated-gauge.h"
#include "buffered-gauge.h"
#include "compass-gauge.h"
#include "generic-layer.h"

static void compass_gauge_render_value(CompassGauge *self, float value);
static AnimatedGaugeOps compass_gauge_ops = {
   .render_value = (ValueRenderFunc)compass_gauge_render_value
};

CompassGauge *compass_gauge_new(void)
{
    CompassGauge *rv;
    rv = calloc(1, sizeof(CompassGauge));
    if(rv){
        if(!compass_gauge_init(rv)){
            free(rv);
            return NULL;
        }
    }
    return(rv);
}

CompassGauge *compass_gauge_init(CompassGauge *self)
{
    bool rv;


    rv = generic_layer_init_from_file(&self->outer, "compass-outer.png");
    if(!rv){
        printf("Couldn't load compass outer ring\n");
        return NULL;
    }
    rv = generic_layer_init_from_file(&self->inner, "compass-inner.png");
    if(!rv){
        printf("Couldn't load compass inner ring\n");
        return NULL;
    }

    animated_gauge_init(ANIMATED_GAUGE(self),
        ANIMATED_GAUGE_OPS(&compass_gauge_ops),
        generic_layer_w(&self->outer),
        generic_layer_h(&self->outer)
    );
    BUFFERED_GAUGE(self)->max_ops = 2;

    generic_layer_build_texture(&self->outer);
    generic_layer_build_texture(&self->inner);

    return self;
}

void compass_gauge_dispose(CompassGauge *self)
{
    generic_layer_dispose(&self->outer);
    generic_layer_dispose(&self->inner);

    animated_gauge_dispose(ANIMATED_GAUGE(self));
}

void compass_gauge_free(CompassGauge *self)
{
    compass_gauge_dispose(self);
    free(self);
}

static void compass_gauge_render_value(CompassGauge *self, float value)
{
    SDL_Point icenter;
    SDL_Rect dst;

    icenter = (SDL_Point){
        .x = (generic_layer_w(&self->inner) - 1)/2,
        .y = (generic_layer_h(&self->inner) - 1)/2,
    };

    dst = (SDL_Rect){
        .x = (generic_layer_w(&self->outer) - 1)/2 - icenter.x,
        .y = (generic_layer_h(&self->outer) - 1)/2 - icenter.y,
        .w = generic_layer_w(&self->inner),
        .h = generic_layer_h(&self->inner)
    };

    buffered_gauge_blit_layer(BUFFERED_GAUGE(self), &self->outer, NULL, NULL);
    buffered_gauge_blit_rotated_texture(BUFFERED_GAUGE(self),
        self->inner.texture,
        NULL,
        value * -1.0,
        &icenter,
        &dst,
        NULL);
}

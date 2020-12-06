#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#include "base-animation.h"
#include "base-gauge.h"
#include "digit-barrel.h"
#include "odo-gauge.h"
#include "sdl-colors.h"


#define SPIN_DURATION 1000 //millisecond

static void odo_gauge_render(OdoGauge *self, Uint32 dt, RenderContext *ctx);
static void odo_gauge_update_state(OdoGauge *self, Uint32 dt);

static BaseGaugeOps odo_gauge_ops = {
   .render = (RenderFunc)odo_gauge_render,
   .update_state = (StateUpdateFunc)odo_gauge_update_state
};


/**
 * Creates a new odometer-like gauge.
 *
 *
 * @param height height in pixels or -1 to match symbol size
 * @param rubis where to place the indicator line or -1 to put it
 * in the middle
 *
 *
 */
OdoGauge *odo_gauge_new(DigitBarrel *barrel, int height, int rubis)
{
    OdoGauge *self;

    self = calloc(1, sizeof(OdoGauge));
    if(self){
        if(!odo_gauge_init(self, rubis, 1, height, barrel)){
            free(self);
            return NULL;
        }
    }
    return self;
}

OdoGauge *odo_gauge_new_multiple(int rubis, int nbarrels, ...)
{
    OdoGauge *self, *rv;
    va_list args;


    self = calloc(1, sizeof(OdoGauge));
    if(self){
        va_start(args, nbarrels);
        rv = odo_gauge_vainit(self, rubis, nbarrels, args);
        va_end(args);
        if(!rv){
            free(self);
            return NULL;
        }
    }
    return self;
}


OdoGauge *odo_gauge_init(OdoGauge *self, int rubis, int nbarrels, ...)
{
    OdoGauge *rv;
    va_list args;

    va_start(args, nbarrels);
    rv = odo_gauge_vainit(self, rubis, nbarrels, args);
    va_end(args);

    return rv;
}

OdoGauge *odo_gauge_vainit(OdoGauge *self, int rubis, int nbarrels, va_list ap)
{
    int width;
    int max_height;

    self->nbarrels = nbarrels;
    self->barrels = calloc(self->nbarrels, sizeof(DigitBarrel*));
    self->heights = calloc(self->nbarrels, sizeof(int));

    width = 0;
    max_height = 0;
    for(int i = 0, pwr = 0; i < nbarrels; i++){
        self->heights[i] = va_arg(ap, int);
        self->barrels[i] = va_arg(ap, DigitBarrel*);
        self->barrels[i]->refcount++;

        width += generic_layer_w(GENERIC_LAYER(self->barrels[i]));
        if(self->heights[i] == -1)
            self->heights[i] = self->barrels[i]->symbol_h*2;
        else if(self->heights[i] == -2)
            self->heights[i] = self->barrels[i]->symbol_h;
        max_height = (self->heights[i] > max_height) ? self->heights[i] : max_height;

        float val = floor(VERTICAL_STRIP(self->barrels[i])->end) * powf(10.0,pwr);
        self->max_value += val;
        pwr += number_digits(VERTICAL_STRIP(self->barrels[i])->end);
    }

    self->state.barrel_states = calloc(self->nbarrels, sizeof(DigitBarrelState));
    self->state.fill_rects = calloc(self->nbarrels, sizeof(SDL_Rect));
#if 0
    void *rv = animated_gauge_init(ANIMATED_GAUGE(self), ANIMATED_GAUGE_OPS(&odo_gauge_ops), width, max_height);
#else
    void *rv = base_gauge_init(BASE_GAUGE(self), &odo_gauge_ops, width, max_height);
#endif
    if(!rv){
        free(self->barrels);
        return NULL;
    }
    if(rubis > 0)
        self->rubis = rubis;
    else
        self->rubis = round(base_gauge_h(BASE_GAUGE(self))/2.0);

    return self;
}


void odo_gauge_free(OdoGauge *self)
{
    for(int i = 0; i < self->nbarrels; i++){
        digit_barrel_free(self->barrels[i]);
    }
    free(self->barrels);
    free(self->heights);
    if(self->state.barrel_states)
        free(self->state.barrel_states);
    if(self->state.fill_rects)
        free(self->state.fill_rects);
    base_gauge_dispose(BASE_GAUGE(self));
    free(self);
}

bool odo_gauge_set_value(OdoGauge *self, float value, bool animated)
{
    bool rv = true;
    BaseAnimation *animation;

    if(value > self->max_value){
        value = self->max_value;
        rv = false;
    }

    if(animated){
        if(BASE_GAUGE(self)->nanimations == 0){
            animation = base_animation_new(TYPE_FLOAT, 1, &self->value);
            base_gauge_add_animation(BASE_GAUGE(self), animation);
            base_animation_unref(animation);/*base_gauge takes ownership*/
        }else{
            animation = BASE_GAUGE(self)->animations[0];
        }
        base_animation_start(animation, self->value, value, SPIN_DURATION);
    }else{
        self->value = value;
        BASE_GAUGE(self)->dirty = true;
    }

    return rv;
}

static void odo_gauge_render(OdoGauge *self, Uint32 dt, RenderContext *ctx)
{
    DigitBarrelState *bstate;
    for(int i = 0; i < self->state.nbarrel_states; i++){
        bstate = &self->state.barrel_states[i];
        for(int j = 0; j < bstate->npatches; j++)
            base_gauge_blit_layer(BASE_GAUGE(self), ctx, bstate->layer, &bstate->patches[j].src, &bstate->patches[j].dst);
    }
    for(int i = 0; i < self->state.nfill_rects; i++){
        base_gauge_fill(BASE_GAUGE(self), ctx, &self->state.fill_rects[i], &SDL_BLACK, false);
    }
    base_gauge_draw_rubis(BASE_GAUGE(self), ctx, self->state.rubis_y,
                          &SDL_RED, self->state.pskip);
}

static void odo_gauge_update_state(OdoGauge *self, Uint32 dt)
{
    float vparts[6]; /*up to 999.999 ft*/
    int nparts;
    int current_part = 0;
    int current_rotor = 0;
    int current_rotor_rank = 0;
    float current_val;
    int next_part;
    int i;
    int rcenter;
    SDL_Rect cursor;

    self->state.nbarrel_states = 0;
    self->state.nfill_rects = 0;

    cursor = (SDL_Rect){
        .x = base_gauge_w(BASE_GAUGE(self)),
        .y = 0,
        .w = base_gauge_w(BASE_GAUGE(self)),
        .h = base_gauge_h(BASE_GAUGE(self))
    };

    /* If the buffer is shared, it's up to the "parent"
     * to clear portions when appropriate
     * */
//    if(BUFFERED_GAUGE(self)->type == BUFFER_OWN)
//        buffered_gauge_clear(BUFFERED_GAUGE(self));

    nparts = number_split(self->value, vparts, 6);
//    printf("doing value %f, splitted in to %d parts\n",value,nparts);
    do{
//        current_rotor_rank = current_rotor;
        current_rotor_rank += floor(log10(VERTICAL_STRIP(self->barrels[current_rotor])->end));
//        printf("current_part: %d, current_rotor_rank: %d\n",current_part,current_rotor_rank);
        if(current_part == current_rotor_rank){
            current_val = vparts[current_part];
            next_part = current_part + 1;
        }else{
            current_val = 0;
            for(i = current_part; i <= current_rotor_rank && i < nparts ; i++)
                current_val += vparts[i] * powf(10.0, i);
            next_part = i;
        }
        cursor.x -= generic_layer_w(GENERIC_LAYER(self->barrels[current_rotor]));
        cursor.h = self->heights[current_rotor];
        rcenter = (base_gauge_h(BASE_GAUGE(self))/2 - cursor.h/2); /*This is the rotor center relative to the whole gauge size*/
        cursor.y = 0 + rcenter;
        cursor.w = generic_layer_w(GENERIC_LAYER(self->barrels[current_rotor]));
        memset(&self->state.barrel_states[current_rotor], 0, sizeof(DigitBarrelState));
        digit_barrel_state_value(self->barrels[current_rotor], current_val, &cursor, self->rubis - rcenter, &self->state.barrel_states[current_rotor]);
        self->state.nbarrel_states++;
//        printf("setting rotor %d to %f\n",current_rotor, current_val);
        //render that value
        //next part, next rotor
        current_part = next_part;
        current_rotor++;
        current_rotor_rank++;
//        printf("current_part: %d, nparts: %d\n",current_part,nparts);
    }while(current_part < nparts);

    /*Place holders for rotors that didn't render any value*/
    for(; current_rotor < self->nbarrels; current_rotor++){
        cursor.x -= generic_layer_w(GENERIC_LAYER(self->barrels[current_rotor]));
        cursor.h = self->heights[current_rotor];
        cursor.y = 0 + base_gauge_h(BASE_GAUGE(self))/2 - cursor.h/2;
        cursor.w = generic_layer_w(GENERIC_LAYER(self->barrels[current_rotor]));

        self->state.fill_rects[self->state.nfill_rects] = cursor;
        self->state.nfill_rects++;
    }
    self->state.rubis_y = self->rubis;
    self->state.pskip = round(base_gauge_w(BASE_GAUGE(self))/2.0);
}

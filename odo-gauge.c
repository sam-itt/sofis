#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#include "animated-gauge.h"
#include "sdl-colors.h"
#include "odo-gauge.h"


#define SPIN_DURATION 1000 //millisecond


void odo_gauge_render_value(OdoGauge *self, float value);

static AnimatedGaugeOps odo_gauge_ops = {
   .render_value = (ValueRenderFunc)odo_gauge_render_value
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

        width += VERTICAL_STRIP(self->barrels[i])->ruler->w;
        if(self->heights[i] == -1)
            self->heights[i] = self->barrels[i]->symbol_h*2;
        else if(self->heights[i] == -2)
            self->heights[i] = self->barrels[i]->symbol_h;
        max_height = (self->heights[i] > max_height) ? self->heights[i] : max_height;

        float val = floor(VERTICAL_STRIP(self->barrels[i])->end) * powf(10.0,pwr);
        self->max_value += val;
        pwr += number_digits(VERTICAL_STRIP(self->barrels[i])->end);
    }

    void *rv = animated_gauge_init(ANIMATED_GAUGE(self), ANIMATED_GAUGE_OPS(&odo_gauge_ops), width, max_height);
    if(!rv){
        free(self->barrels);
        return NULL;
    }
    if(rubis > 0)
        self->rubis = rubis;
    else
        self->rubis = round(BASE_GAUGE(self)->h/2.0);

    return self;
}


void odo_gauge_free(OdoGauge *self)
{
    for(int i = 0; i < self->nbarrels; i++){
        digit_barrel_free(self->barrels[i]);
    }
    free(self->barrels);
    free(self->heights);
    animated_gauge_dispose(ANIMATED_GAUGE(self));
    free(self);
}

bool odo_gauge_set_value(OdoGauge *self, float value)
{
    bool rv = true;

    if(value > self->max_value){
        value = self->max_value;
        rv = false;
    }
    animated_gauge_set_value(ANIMATED_GAUGE(self), value);

    return rv;
}


static void odo_gauge_draw_rubis(OdoGauge *self)
{
    SDL_Surface *gauge;

    gauge = ANIMATED_GAUGE(self)->view;

    SDL_LockSurface(gauge);

    Uint32 *pixels = gauge->pixels;
    Uint32 color = SDL_URED(gauge);
    int empty_pixels = gauge->w / 2;
    int stop_idx = round(empty_pixels/2.0);
    int restart_idx = round(gauge->w - empty_pixels/2.0);
    for(int x = 0; x < gauge->w; x++){
        if(!empty_pixels || x < stop_idx || x >= restart_idx)
            pixels[self->rubis * gauge->w + x] = color;
    }
    SDL_UnlockSurface(gauge);
}


void odo_gauge_render_value(OdoGauge *self, float value)
{
    float vparts[6]; /*up to 999.999 ft*/
    int nparts;
    int current_part = 0;
    int current_rotor = 0;
    int current_rotor_rank = 0;
    float current_val;
    int next_part;
    int i;
    SDL_Surface *gauge;
    int rcenter;
    SDL_Rect cursor;

    gauge = ANIMATED_GAUGE(self)->view;

    cursor = (SDL_Rect){BASE_GAUGE(self)->w,0,BASE_GAUGE(self)->w,BASE_GAUGE(self)->h};

    //SDL_FillRect(gauge, NULL, SDL_UCKEY(gauge));
    animated_gauge_clear(ANIMATED_GAUGE(self));

    nparts = number_split(value, vparts, 6);
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
        cursor.x -= VERTICAL_STRIP(self->barrels[current_rotor])->ruler->w;
        cursor.h = self->heights[current_rotor];
        rcenter = (BASE_GAUGE(self)->h/2 - cursor.h/2); /*This is the rotor center relative to the whole gauge size*/
        cursor.y = 0 + rcenter;
        cursor.w = VERTICAL_STRIP(self->barrels[current_rotor])->ruler->w;
        digit_barrel_render_value(self->barrels[current_rotor], current_val, gauge, &cursor, self->rubis - rcenter);
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
        cursor.x -= VERTICAL_STRIP(self->barrels[current_rotor])->ruler->w;
        cursor.h = self->heights[current_rotor];
        cursor.y = 0 + BASE_GAUGE(self)->h/2 - cursor.h/2;
        cursor.w = VERTICAL_STRIP(self->barrels[current_rotor])->ruler->w;

        SDL_FillRect(gauge, &cursor, SDL_UBLACK(gauge));
    }
    odo_gauge_draw_rubis(self);
}


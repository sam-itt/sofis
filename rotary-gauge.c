#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_image.h>

#include "rotary-gauge.h"

#define SPEED 50 /*pixel per second*/
#define SPIN_DURATION 1000 //millisecond
#define LIMIT 99
#define SYMBOL_H 61.0

#define MSEC_TO_SEC(a) ((a)/1000.0)

static void value_spinner_draw_value_marks(ValueSpinner *self)
{
    int iy;
    SDL_LockSurface(self->values);

    Uint32 *pixels = self->values->pixels;
    Uint32 color = SDL_MapRGB(self->values->format, 0xFF, 0xFF, 0x00);
    float count = 0;
    for(float y = self->fvo; y < self->values->h; y += self->symbol_h/2.0){
        iy = round(y);
//        printf("%0.2f y = %d\n",count,iy);
        for(int x = 0; x < self->values->w; x++){
            pixels[iy * self->values->w + x] = color;
        }
        count += 5.0;
    }
    SDL_UnlockSurface(self->values);
}


ValueSpinner *value_spinner_init(ValueSpinner *self, const char *filename, float fvo, float symbol_h, float start, float end)
{
    self->values = IMG_Load(filename);
    if(!self->values)
        return NULL;
    
    self->fvo = fvo;
    self->symbol_h = symbol_h;
    self->start = start;
    self->end = end;
    self->ppv = self->values->h/(self->end - self->start + 1.0);
    value_spinner_draw_value_marks(self);
    return self;
}



/*
 * Rubis is the offset in dst where to align the value fomr the spinner.
 * if its negative, the rubis will be (vertical) the center dst: the y index
 * in the value spinner representing @param value will be aligned with the middle
 *
 */

void value_spinner_render_value(ValueSpinner *self, float value, SDL_Surface *dst, float rubis)
{
    float y;
    SDL_Rect dst_region = {0,0,0,0};

    /*translate @param value to an index in the spinner texture*/
    y =  value * self->ppv + self->fvo;
//    printf("%0.2f y = %f\n",value, round(y));
    y = round(y);
    rubis = (rubis < 0) ? dst->h / 2.0 : rubis;
    SDL_Rect portion = {
        .x = 0, 
        .y = round(y - rubis),
        .w = self->values->w,
        .h = dst->h
    };

    if(portion.y < 0){ //Fill top
        SDL_Rect patch = {
            .x = 0,
            .y = self->values->h + portion.y, //means - portion.y as portion.y < 0 here
            .w = self->values->w,
            .h = self->values->h - patch.y
        };
        SDL_BlitSurface(self->values, &patch, dst, &dst_region);
        dst_region.y = patch.h;
        portion.y = 0;
        portion.h -= patch.h;
    }
    SDL_BlitSurface(self->values, &portion, dst, &dst_region);
    if(portion.y + dst->h > self->values->h){// fill bottom
        float taken = self->values->h - portion.y; //number of pixels taken from the bottom of values pict
        float delta = dst->h - taken;
        dst_region.y += taken; 
        SDL_Rect patch = {
            .x = 0,
            .y = 0,
            .w = self->values->w,
            .h = delta
        };
        SDL_BlitSurface(self->values, &patch, dst, &dst_region);
    }
}


RotaryGauge *rotary_gauge_new(int rubis)
{
    RotaryGauge *self;
    ValueSpinner *spin;

    self = calloc(1, sizeof(RotaryGauge));
    if(self){
        self->gauge = SDL_CreateRGBSurface(0, 64, 61+61, 32, 0, 0, 0, 0);
        spin = value_spinner_init(&self->spinner, "rotaryXX.png", 30.5, 61, 0, 99);
        self->damaged = true;
        if(!self->gauge || !spin){
            free(self);
            return NULL;
        }
        if(rubis > 0)
            self->rubis = rubis;
        else
            self->rubis = round(self->gauge->h/2.0);
//        printf("rubis: %d\n");
//rotary_gauge_set_value(RotaryGauge *self, float value)
    }
    return self;
}

#if 0
static float rotary_gauge_get_speed(RotaryGauge *self)
{
    float dist;

    dist = self->value - self->_start_val;
    if(self->_current_frame_value/dist < 0.8){ // we didn't made 80% of the distance yet
        printf("low speed\n");
        return dist/(0.8*SPIN_DURATION); 
    }
    printf("high speed\n");
    return dist/(0.2*SPIN_DURATION);
}
#endif

void rotary_gauge_set_value(RotaryGauge *self, float value)
{
    basic_animation_start(&self->animation, self->value, value, SPIN_DURATION); 
    self->value = value;
}


static void rotary_gauge_draw_rubis(RotaryGauge *self)
{
    SDL_LockSurface(self->gauge);

    Uint32 *pixels = self->gauge->pixels;
    Uint32 color = SDL_MapRGB(self->gauge->format, 0xFF, 0x00, 0x00);
    int empty_pixels = self->gauge->w / 2;
    int stop_idx = round(empty_pixels/2.0);
    int restart_idx = round(self->gauge->w - empty_pixels/2.0);
    for(int x = 0; x < self->gauge->w; x++){
        if(!empty_pixels || x < stop_idx || x >= restart_idx) 
            pixels[self->rubis * self->gauge->w + x] = color;
    }
    SDL_UnlockSurface(self->gauge);
}

/*dt in msec*/
SDL_Surface *rotary_gauge_render(RotaryGauge *self, Uint32 dt)
{
    float _current;

    if(self->animation.current != self->value || self->damaged){
        _current = basic_animation_loop(&self->animation, dt);

        value_spinner_render_value(&self->spinner, _current, self->gauge, self->rubis);
        rotary_gauge_draw_rubis(self);
        self->damaged = false;
    }
    return self->gauge;
}

#ifndef ROTARY_GAUGE_H
#define ROTARY_GAUGE_H

#include <stdbool.h>

#include <SDL2/SDL.h>

#include "basic-animation.h"

typedef struct{
    SDL_Surface *values;
    float fvo; //first value offset
    float symbol_h; //symbol (i.e digits) height in pixels
    float start, end; //range
    float ppv; /* pixels per value*/
}ValueSpinner;


typedef struct{
    SDL_Surface *gauge;
    ValueSpinner spinner;
//    SDL_Surface *values;
//    SDL_Rect portion;

    int rubis;
    float value;
    
    bool damaged;
    BasicAnimation animation;
/*    float _start_val;
    float _current_frame_value;
    float _time_progress;*/
}RotaryGauge;

RotaryGauge *rotary_gauge_new(int rubis);

SDL_Surface *rotary_gauge_render(RotaryGauge *self, Uint32 dt);

void rotary_gauge_set_value(RotaryGauge *self, float value);
#endif /* ROTARY_GAUGE_H */

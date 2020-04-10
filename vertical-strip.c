#include <stdio.h>
#include <stdlib.h>

#include "vertical-strip.h"

void vertical_strip_dispose(VerticalStrip *self)
{
    if(self->ruler)
        SDL_FreeSurface(self->ruler);
}

bool vertical_strip_has_value(VerticalStrip *self, float value)
{
    float min, max;

    min = (self->end > self->start) ? self->start : self->end;
    max = (self->end < self->start) ? self->start : self->end;

    return value >= min && value <= max;
}

/**
 * Returns image pixel index (i.e y for vertical gauges, x for horizontal)
 * from a given value
 * TODO be part of generic gauges layer
 *
 * @param value the value to look for
 * @param reverse if true, assumes that 0 is a the bottom of the strip instead of
 * at the top
 * @return the pixel index in {@link #ruler} that maps to @value, or -1 if value can't
 * be mapped (outside of the [{@link #start}, {@link #end}] range)
 */
float vertical_strip_resolve_value(VerticalStrip *self, float value, bool reverse)
{
    float y;

    if(!vertical_strip_has_value(self, value))
        return -1;

    value = fmod(value, fabs(self->end - self->start) + 1);
    y =  round(value * self->ppv + self->fei);
//    printf("VerticalStrip: mapping value %f from [%f %f] to [%d %d]: %f",value, self->start,self->end,0,self->ruler->h-1,y);
    if(reverse){
        y = (self->ruler->h-1) - y;
//        printf(" reversed: %f\n",y);
    }else{
//        printf("\n");
    }

    return y;

}

float vertical_strip_resolve_value_fuzzy(VerticalStrip *self, float value, bool reverse)
{
    float rv;
    /* To map [A, B] --> [a, b]
     *
     * use this formula : (val - A)*(b-a)/(B-A) + a
     *
     */
/*    first interval is start,end, second interval is 0,surface->h-1*/
    if(!reverse){
        rv = (value - self->start)*((self->ruler->h-1) - 0)/(self->end - self->start) + 0;
//        printf("Forward: New resolve: mapping value %f from [%f %f] to [%d %d]: %f\n",value, self->start,self->end,0,self->ruler->h-1,rv);
    }else{
        rv = (value - self->start)*(0 - (self->ruler->h-1))/(self->end - self->start) + (self->ruler->h-1);
//        printf("Reverse: New resolve: mapping value %f from [%f %f] to [%d %d]: %f\n",value, self->start,self->end,self->ruler->h-1,0,rv);
    }
#if 0
    if(reverse){
        rv = (self->ruler->h-1) - rv;
        printf(" reversed: %f\n",rv);
    }else{
        printf("\n");
    }
#endif
    return rv;
}

float vertical_strip_resolve_value_new(VerticalStrip *self, float value, float first_grad, bool reverse)
{
/*
    if(!vertical_strip_has_value(self, value))
        return -1;*/

    if(fmod(value, self->vstep) == 0){ /*Value is a big graduation*/
        float y;
        value -= first_grad;
        int ngrads = value/self->vstep;
        if(!reverse)
            y = self->fei + ngrads * self->ppv * self->vstep;
        else
            y = self->fei - ngrads * self->ppv * self->vstep;
        return y;
    }else if(self->vsubstep != 0 && fmod(value, self->vsubstep) == 0){ /*Value is a small graduation*/
        float y;
        value -= first_grad;
        int ngrads = value/self->vsubstep;
        if(!reverse)
            y = self->fei + ngrads * self->ppv * self->vsubstep;
        else
            y = self->fei - ngrads * self->ppv * self->vsubstep;
        return y;
    }else{
        return vertical_strip_resolve_value_fuzzy(self, value, reverse);
    }
}


void vertical_strip_clip_value(VerticalStrip *self, float *value)
{
    if(*value > self->end)
        *value = self->end;
    if(*value < self->start)
        *value = self->start;
}

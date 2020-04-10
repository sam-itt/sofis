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
 * from a given value. This function does a crude approximation that might fail
 * to align with etch marks, if any. If there are any etch marks on the strip,
 * derivative classes should handle them and call this function only to resolve
 * values between marks
 *
 * @param value the value to look for
 * @param reverse if true, assumes that 0 is a the bottom of the strip instead of
 * at the top
 * @return the pixel index in {@link #ruler} that maps to @value, or -1 if value can't
 * be mapped (outside of the [{@link #start}, {@link #end}] range)
 */
float vertical_strip_resolve_value(VerticalStrip *self, float value, bool reverse)
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
    return rv;
}

void vertical_strip_clip_value(VerticalStrip *self, float *value)
{
    if(*value > self->end)
        *value = self->end;
    if(*value < self->start)
        *value = self->start;
}

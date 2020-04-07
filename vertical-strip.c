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

    y =  round(value * self->ppv + self->fvo);
    if(reverse)
        y = (self->ruler->h-1) - y;

    return y;
}

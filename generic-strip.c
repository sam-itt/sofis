/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include "generic-layer.h"
#include "generic-strip.h"

void generic_strip_dispose(GenericStrip *self)
{
    generic_layer_dispose(GENERIC_LAYER(self));
}

bool generic_strip_has_value(GenericStrip *self, float value)
{
    float min, max;

    min = (self->end > self->start) ? self->start : self->end;
    max = (self->end < self->start) ? self->start : self->end;

    return value >= min && value <= max;
}

/**
 * Returns image pixel index (i.e y for vertical strips, x for horizontal)
 * from a given value. This function does a crude approximation that might fail
 * to align with etch marks, if any. If there are any etch marks on the strip,
 * derivative classes should handle them and call this function only to resolve
 * values between marks
 *
 * @param value the value to look for
 *
 * @return the pixel index in {@link #ruler} that maps to @value, or -1 if value can't
 * be mapped (outside of the [{@link #start}, {@link #end}] range)
 */
float generic_strip_resolve_value(GenericStrip *self, float value)
{
    float rv;
    /* To map [A, B] --> [a, b]
     *
     * use this formula : (val - A)*(b-a)/(B-A) + a
     *
     */
/*    first interval is start,end, second interval is 0,surface->h-1*/
    int axis_max = (self->flags & ORIENTATION_MASK) == ORIENTATION_VERTICAL
                   ? generic_layer_h(GENERIC_LAYER(self)) - 1
                   : generic_layer_w(GENERIC_LAYER(self)) - 1;
    bool reverse = (self->flags & DIRECTION_MASK) == GROW_AGAINST_AXIS;
    if(!reverse){
        rv = (value - self->start)*(axis_max - 0)/(self->end - self->start) + 0;
//        printf("Forward: New resolve: mapping value %f from [%f %f] to [%d %d]: %f\n",value, self->start,self->end,0,axis_max,rv);
    }else{
        rv = (value - self->start)*(0 - axis_max)/(self->end - self->start) + axis_max;
//        printf("Reverse: New resolve: mapping value %f from [%f %f] to [%d %d]: %f\n",value, self->start,self->end,axis_max,0,rv);
    }
    return rv;
}

void generic_strip_clip_value(GenericStrip *self, float *value)
{
    if(*value > self->end)
        *value = self->end;
    if(*value < self->start)
        *value = self->start;
}

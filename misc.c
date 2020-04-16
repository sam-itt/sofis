#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "misc.h"

/**
 * Separate a base-10 number into its components: units,
 * tens, hundreds, etc. The values will be laid out as
 * n = parts[0] * 10^0 + parts[1]*10^2 ... + parts[x]*10^x
 *       (units)         (tens)        ...    (rank x)
 * i.e, the index in the array is the rank of the value
 * stored at that index.
 *
 * @param n the value to split out
 * @parts parts a pointer to a float array with enough
 * space to hold all splitted out digits.
 * @param p_size number of parts that @param parts can store
 * @return the number of base-10 ranks that have been found
 *
 */
int number_split(float n, float *parts, size_t p_size)
{
    int i = 1;
    float val;
    size_t rank = 0;
    int divider;

    memset(parts, 0, sizeof(float)*p_size);
    while(n > 0 && rank < p_size) {
        val = fmod(n, 10) * i;
        divider = pow(10, rank);
        parts[rank] = val /= divider;
//        printf("val: %f\n", val);
        n = floor(n / 10);
        i *= 10;
        rank++;
    }
    return rank;
}

/**
 * Count the number of digits in n
 *
 */
int number_digits(float n)
{
    int i = 1;
    size_t rank = 0;

    while(n > 0 ) {
        n = floor(n / 10);
        i *= 10;
        rank++;
    }
    return rank;
}

bool interval_intersect(float as, float ae, float bs, float be, float *is, float *ie)
{
    if(bs > ae || as > be)
        return false;

    *is = (as > bs) ? as : bs;
    *ie = (ae < be) ? ae : be;

    return true;
}

/**
 * Centers self on/in the reference rectangle. Self width
 * and height *must* be set.
 *
 * @param self the SDL_Rect that is worked on. Must have w and h set.
 * @param reference the rectangle that will have self in its center
 */

void SDLExt_RectCenter(SDL_Rect *self, SDL_Rect *reference)
{
    self->y = reference->y + round(reference->h/2.0) - round(self->h/2.0) -1;
    self->x = reference->x + round(reference->w/2.0) - round(self->w/2.0) -1;
}

void SDLExt_RectDump(SDL_Rect *self)
{
    printf("SDL_Rect(%p): x:%d, y:%d, w:%d, h:%d\n",self,self->x,self->y,self->w,self->h);
}

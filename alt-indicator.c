#include <stdio.h>
#include <stdlib.h>

#include "alt-indicator.h"

AltIndicator *alt_indicator_new(void)
{
    AltIndicator *self;
    self = calloc(1, sizeof(AltIndicator));
    if(self){
        self->ladder = ladder_gauge_new(BOTTUM_UP, -1);

        DigitBarrel *db = digit_barrel_new(18, 0, 9.999, 1);
        DigitBarrel *db2 = digit_barrel_new(18, 0, 99, 10);
        self->odo = odo_gauge_new_multiple(-1, 4,
                -1, db2,
                -2, db,
                -2, db,
                -2, db
        );
    }
    return self;
}

void alt_indicator_free(AltIndicator *self)
{
    ladder_gauge_free(self->ladder);
    odo_gauge_free(self->odo);
    free(self);
}

bool alt_indicator_set_value(AltIndicator *self, float value)
{

    animated_gauge_set_value(ANIMATED_GAUGE(self->ladder), value);
    return odo_gauge_set_value(self->odo, value);
}

SDL_Surface *alt_indicator_render(AltIndicator *self, Uint32 dt)
{
    SDL_Surface *rv;
    SDL_Surface *odo;
    SDL_Rect placement;

    rv = animated_gauge_render(ANIMATED_GAUGE(self->ladder), dt);
    if(animated_gauge_moving(ANIMATED_GAUGE(self->ladder)) || animated_gauge_moving(ANIMATED_GAUGE(self->odo))){
        memset(&placement, 0, sizeof(SDL_Rect));
        odo = animated_gauge_render(ANIMATED_GAUGE(self->odo), dt);

        placement.y = 0 + (rv->h-1)/2.0 - odo->h/2.0;
        placement.x = (rv->w - odo->w-1) -1; /*The last -1 in there to prevent eating the border*/
        SDL_BlitSurface(odo, NULL, rv, &placement);
    }
    return rv;
}

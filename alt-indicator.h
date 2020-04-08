#ifndef ALT_INDICATOR_H
#define ALT_INDICATOR_H

#include "ladder-gauge.h"
#include "odo-gauge.h"

typedef struct{
    LadderGauge *ladder;
    OdoGauge *odo;
}AltIndicator;

AltIndicator *alt_indicator_new(void);
void alt_indicator_free(AltIndicator *self);

bool alt_indicator_set_value(AltIndicator *self, float value);

SDL_Surface *alt_indicator_render(AltIndicator *self, Uint32 dt);
uint32_t alt_indicator_get_width(AltIndicator *self);
uint32_t alt_indicator_get_height(AltIndicator *self);
#endif /* ALT_INDICATOR_H */

#ifndef ALT_INDICATOR_H
#define ALT_INDICATOR_H

#include <SDL2/SDL_ttf.h>

#include "ladder-gauge.h"
#include "odo-gauge.h"

typedef enum {ALT_SRC_GPS,ALT_SRC_BARO} AltSource;

typedef struct{
    BaseGauge parent;

    SDL_Surface *view;
    TTF_Font *font;

    LadderGauge *ladder;
    OdoGauge *odo;

    AltSource src;
    float qnh;
    int target_alt;
}AltIndicator;

AltIndicator *alt_indicator_new(void);
AltIndicator *alt_indicator_init(AltIndicator *self);
void alt_indicator_free(AltIndicator *self);

bool alt_indicator_set_value(AltIndicator *self, float value);
void alt_indicator_set_qnh(AltIndicator *self, float value);

#endif /* ALT_INDICATOR_H */

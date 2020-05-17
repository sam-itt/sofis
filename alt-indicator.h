#ifndef ALT_INDICATOR_H
#define ALT_INDICATOR_H

#include "buffered-gauge.h"
#include "ladder-gauge.h"
#include "odo-gauge.h"
#include "text-gauge.h"

typedef enum {ALT_SRC_GPS,ALT_SRC_BARO} AltSource;

typedef struct{
    BufferedGauge super;

    LadderGauge *ladder;
    OdoGauge *odo;
    TextGauge *talt_txt; /*Target altitude*/
    TextGauge *qnh_txt;
    SDL_Surface *gps_flag;
    GPU_Image *tgps_flag;

    AltSource src;
    int qnh;
    int target_alt;
}AltIndicator;

AltIndicator *alt_indicator_new(void);
AltIndicator *alt_indicator_init(AltIndicator *self);
void alt_indicator_free(AltIndicator *self);

bool alt_indicator_set_value(AltIndicator *self, float value);
void alt_indicator_set_qnh(AltIndicator *self, float value);

#endif /* ALT_INDICATOR_H */

#ifndef ELEVATOR_GAUGE_H
#define ELEVATOR_GAUGE_H

#include "animated-gauge.h"
#include "generic-layer.h"
#include "generic-ruler.h"


typedef struct{
    AnimatedGauge super;

    GenericRuler ruler;
    Uint32 color;

    GenericLayer *elevator;
    Location elevator_location;

    ColorZone *zones;
    uint8_t nzones;

    /* Ruler offset within the gauge
     * Cursor base being at y = 0
     * Simple ints would have done but the blitting
     * functions need SDL_Rects
     */
    SDL_Rect ruler_rect;
}ElevatorGauge;

ElevatorGauge *elevator_gauge_new(bool marked, Location elevator_location,
                                  PCF_Font *font, SDL_Color color,
                                  float from, float to, float step,
                                  int bar_max_w, int bar_max_h,
                                  int nzones, ColorZone *zones);
ElevatorGauge *elevator_gauge_init(ElevatorGauge *self,
                                   bool marked, Location elevator_location,
                                   PCF_Font *font, SDL_Color color,
                                   float from, float to, float step,
                                   int bar_max_w, int bar_max_h,
                                   int nzone, ColorZone *zones);


void elevator_gauge_dispose(ElevatorGauge *self);
void elevator_gauge_free(ElevatorGauge *self);
#endif /* ELEVATOR_GAUGE_H */

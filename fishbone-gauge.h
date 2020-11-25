#ifndef FISHBONE_GAUGE_H
#define FISHBONE_GAUGE_H

#include "animated-gauge.h"
#include "generic-layer.h"
#include "generic-ruler.h"

typedef struct{
    AnimatedGauge super;

    GenericRuler ruler;
    Uint32 color;

    /* TODO: Generate cursors otf, and support
     * 2 cursors, one of each side of the scale.
     * */
    GenericLayer *cursor;

    ColorZone *zones;
    uint8_t nzones;

    /* Ruler offset within the gauge
     * Cursor base being at y = 0
     * Simple ints would have done but the blitting
     * functions need SDL_Rects
     */
    SDL_Rect ruler_rect;
    SDL_Rect cursor_rect;
}FishboneGauge;

FishboneGauge *fishbone_gauge_new(bool marked,
                                  PCF_Font *font, SDL_Color color,
                                  float from, float to, float step,
                                  int bar_max_w, int bar_max_h,
                                  int nzones, ColorZone *zones);
FishboneGauge *fishbone_gauge_init(FishboneGauge *self,
                                   bool marked,
                                   PCF_Font *font, SDL_Color color,
                                   float from, float to, float step,
                                   int bar_max_w, int bar_max_h,
                                   int nzone, ColorZone *zones);


void fishbone_gauge_dispose(FishboneGauge *self);
void fishbone_gauge_free(FishboneGauge *self);
#endif /* FISHBONE_GAUGE_H */

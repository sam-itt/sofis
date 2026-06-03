#ifndef VRULER_PAGE_DESCRIPTOR_H

#include "ladder-page.h"

typedef struct{
    LadderPageDescriptor super;

    uint8_t unit_px_sz;
    uint8_t small_step_width;
    uint8_t big_step_width;
}VRulerPageDescriptor;

#define VRULER_PAGE_DESCRIPTOR(self) ((VRulerPageDescriptor*)(self))

VRulerPageDescriptor *vruler_page_descriptor_init(VRulerPageDescriptor *self, int page_w,
                                                  int page_h_min, uintf8_t unit_px_sz,
                                                  int small_step_width, int big_step_width,
                                                  float vstep, float vsubstep,
                                                  ScrollType direction, LPInitFunc func);

LadderPage *vruler_ladder_page_init(LadderPage *self, LadderPageRulerLocation location);
#define VRULER_PAGE_DESCRIPTOR_H
#endif /* VRULER_PAGE_DESCRIPTOR_H */

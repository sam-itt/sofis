#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "vruler-page-descriptor.h"
#include "resource-manager.h"
#include "sizes.h"


/**
 * @brief Initializes a VRulerPageDescriptor that is able to draw
 * arbitrary pages of an infinite ruler ladder.
 *
 * This descriptor will draw a ruler having two levels of markings: big tick
 * marks for big units (graduations) and small tick marks for small units
 * (base units).
 *
 * @param self The VRulerPageDescriptor to initialize
 * @param page_w The width of the page in pixels, that should match
 * the LadderGauge size.
 * @param page_h_min The minimum height of the page in pixels. That should be
 * the height of the LadderGauge, but it will be rounded up to a multiple of a big unit height.
 * @param unit_px_sz The *increment* in pixels from a base unit (small graduations) to the next one,
 * e.g. if a base unit tick is at pixel y = 4 and unit_px_sz is 18, the next base unit tick will
 * be at pixel y = 22. It is *not* the number of pixels *between* two graduations. The number of
 * pixels between two graduations is unit_px_sz - the width of the tick mark, which is currently
 * 1 pixel, thus unit_px_sz - 1.
 * @param small_step_width The width in pixels of the small tick marks.
 * @param big_step_width The width in pixels of the big tick marks.
 * @param vstep The size of a big unit (graduation) in arbitrary units, e.g. 100.
 * @param vsubstep The size of a small unit (base unit) in arbitrary units, e.g. 20.
 * It must be a divisor of vstep, e.g. 100/20 = 5.
 * @param direction The direction of the ladder, either BOTTOM_UP(ok) or TOP_DOWN(untested).
 * @param func The function to use to initialize the LadderPage, e.g. vruler_ladder_page_init
 * if you don't need anything more than drawing the ruler and etching the marks.
 *
 * @return The initialized VRulerPageDescriptor
 */

VRulerPageDescriptor *vruler_page_descriptor_init(VRulerPageDescriptor *self, int page_w,
                                                  int page_h_min, uintf8_t unit_px_sz,
                                                  int small_step_width, int big_step_width,
                                                  float vstep, float vsubstep,
                                                  ScrollType direction, LPInitFunc func)
{
    int pattern_h;

    assert(fmodf(vstep, vsubstep) == 0);

    self->unit_px_sz = unit_px_sz;
    self->big_step_width = big_step_width;
    self->small_step_width = small_step_width;

    /*
     * A small or base unit is vsubtep arbitrary units, such as 20, which will be
     * represented by unit_px_sz.
     *
     * A big unit or graduation is vstep arbitrary units, such as 100. In this
     * example a big unit is made of 5 small units.
     */
    int small_units_per_big_unit = vstep/vsubstep;

    /* The final height will be a multiple of a "stitchable pattern" size.
     *
     * A "stitchable pattern" consists of half a big unit plus leading and
     * trailing pixels so that if a pattern was stitched onto itself, leading
     * pixels connected to trailing pixels, that would make 1 big unit.
     *
     * The size of a pattern is equivalent to one big unit.
     */
    pattern_h = unit_px_sz * small_units_per_big_unit;
    int page_h = round_up(page_h_min, pattern_h);
    printf("Ruler ladder page height: %d, computed from: %d as a multiple of %d\n", page_h, page_h_min, pattern_h);
    int npatterns = page_h/pattern_h;

    /*A page will represent page_size arbitrary unit values, e.g. 70. Those are not pixels*/
    float page_size = npatterns * vstep;
    printf("Ruler ladder page size: %f, computed from: %d patterns of %d pixels\n", page_size, npatterns, pattern_h);


    ladder_page_descriptor_init(
        LADDER_PAGE_DESCRIPTOR(self),
        page_w, page_h,
        direction /*BOTTUM_UP*/, page_size, vstep /*100, big/graduation unit*/, vsubstep /*20, small/base unit*/,
        func
    );

    /* First big tick mark is at the end of the first pattern, which is page_h - unit_px_sz
     * page_h - 2 unit - ceil(1/2 unit) from the bottom (0)
     *
     * */
    LADDER_PAGE_DESCRIPTOR(self)->fei = page_h - 2 * unit_px_sz - ceil(unit_px_sz/2.0);
    printf("Ruler ladder page first etch mark index: %f\n", LADDER_PAGE_DESCRIPTOR(self)->fei);

    return self;
}

/**
 * @brief Draws the ruler and etches the markings on the page.
 *
 * This is where the drawing is done for a new page. Chain-up to it in
 * your subclass (@see airspeed_ladder_page_init, altitude_ladder_page_init)
 *
 * @param self The LadderPage to initialize.
 *
 * @return The initialized LadderPage with the ruler and markings drawn.
 */
LadderPage *vruler_ladder_page_init(LadderPage *self)
{
    VRulerPageDescriptor *descriptor;

    descriptor = (VRulerPageDescriptor *)self->descriptor;

    ladder_page_init(self);
    ladder_page_draw_ruler(self, descriptor->unit_px_sz, LocationRight,
                           descriptor->small_step_width, descriptor->big_step_width);
    ladder_page_etch_markings(self, resource_manager_get_font(FONT_BIG), HALIGN_RIGHT, descriptor->big_step_width + 5);

    return self;
}



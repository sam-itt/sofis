/*
 * SPDX-FileCopyrightText: 2026 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>

#include "ladder-page-descriptor.h"
#include "ladder-page.h"

/**
 * LadderPageDescriptor is an abstract class that provide a basis on which
 * derived classes can implement a specific ladder page descriptor.
 *
 * LadderPageDescriptor works together with LadderPage to define how the
 * infinite strip is made out of pages. LadderPageDescriptor which derives from
 * RulerCore provides the information that is shared by all pages like the tick
 * mark pixel positions while LadderPages store their specific information like
 * their actual range (an exception to that are the direction/orientation flags
 * that albeit being the same for all pages is still stored in LadderPage
 * GenericStrip ancestor).
 *
 * The pages' GenericStrip image must in fact be larger than the pure @c
 * page_size: For a page_size of 70, 0 cannot be the very first pixel, and 69
 * cannot be the very last as this wouldn't allow for:
 *  a) putting a number in front of the mark: The digit being centered on
 *     the mark, there wouldn't be enough space for the lower/upper part as
 *     the digit centerline would be right at 0 or max-1
 *  b) chaining up the pages: the strip is built by concatenating pages
 *     one right after/before the other. Having the min/max marks right at
 *     0/max pixels would result in eating up an interval: Imagine page 0
 *     being *values* 0-9 and page 2 being *values* 10-19, with graduations
 *     every unit. It's safe to assume that the ruler image will have a fixed
 *     amount of pixels between each mark.
 *     A direct contactenation would have 9 and 10 with absolutely no interval
 *     in-between which doesn't work.
 * Therefore the image needs to be larger than the interval. It needs to have a
 * leading and trailing area that together account for the same space as in
 * between two of the biggest ticks. i.e if those ticks are 6 pixels appart, you
 * need 3 leading pixels and 3 trailing pixels. This will allow for seamless
 * concatenatation. WARNING: If it's not possible to have an equal amount of
 * leading/trailing pixels, LEADING area must get the HIGHER number of pixels.
 * Example: 7 pixels spacing between values, hence 3.5 pixels which cannot be
 * done. 4 leading pixels, 3 trailing. See ladder_page_descriptor_init for more
 * details.
 *
 * In a nutshell, LadderPages will have some leading/trailing unlabeld ticks so
 * that they can be stitched together: A page won't start at 700 mark because
 * that would be the first (or last depending on the direction) pixel of the
 * image and thus half the label would be cut off, making page drawing more
 * complex. That leading/trailing @c offset will be computed by
 * LadderPageDescriptor init.
 *
 * Either if you use a loaded image or generate it on the fly, you must follow
 * this rule.
 *
 * @see AirspeedPageDescriptor
 * @see AltitudePageDescriptor
 */


/**
 * @brief Initialize a LadderPageDescriptor with the given parameters. This
 * function should be chained-up to by derived classes to initialize the base
 * class.
 *
 *
 * @param self The LadderPageDescriptor to initialize.
 * @param width The width of the page in pixels.
 * @param height_min The minimum height of the page in pixels. This function
 * will round up the height to the nearest multiple of the pattern size.
 * @param direction The direction of the ladder page (BOTTUM_UP or TOP_DOWN).
 * @param base_unit The base unit in arbitrary units. This is the smallest unit
 * that will be represented on the page (e.g. 20 will have graduations every 20
 * units).
 * @param base_unit_px The size of the base unit in pixels. This is the number
 * of pixels that will be used to represent the base unit on the page.
 * @param base_unit_size The size of the base unit tick in pixels. This is the
 * unit tick mark.
 * @param base_unit_labeled Whether the base unit tick will get a value label
 * (number) drawn or not.
 * @param nmultiples The number of multiples of the base unit that follows.
 * @param multiples The "overunit" ticks that will be drawn on the page, e.g. 5
 * will draw a bigger tick every 5 base units. @see RulerCore for more details.
 * @param LPInitFunc func The function that will be called to initialize a
 * LadderPage.
 *
 * @return The initialized LadderPageDescriptor.
 *
 * @see RulerCore
 * @see ruler_core_init
 * @see RulerUnitMultiple
 */
LadderPageDescriptor *ladder_page_descriptor_init(LadderPageDescriptor *self, int width, int height_min,
                                                  ScrollType direction, float base_unit, uint16_t base_unit_px,
                                                  uint8_t base_unit_size, bool base_unit_labeled,
                                                  uint8_t nmultiples, RulerUnitMultiple *multiples,
                                                  LPInitFunc func)
{

    int pattern_h;

    /* A LadderPage must start/stop so that the labels can be drawn on the image
     * and thus we cannot start/end abruptly at a big tick mark. We need
     * leading and trailing pixels so that the first/last labels can be drawn
     * without being cut off.
     * here we assume that big ticks are always labeled and that the font is
     * smaller than half a big tick "stride".
     *
     * TODO: Having that fully computed will be nice.
     */

    /* A small or base unit is vsubtep arbitrary units, such as 20, which will
     * be represented by unit_px_sz.
     *
     * A big unit or graduation is vstep arbitrary units, such as 100. In this
     * example a big unit is made of 5 small units.
     */
    int big_unit = nmultiples > 0 ? multiples[nmultiples-1].value * base_unit : base_unit;
    int small_units_per_big_unit = multiples[nmultiples-1].value;

    /* The final height will be a multiple of a "stitchable pattern" size.
     *
     * A "stitchable pattern" consists of half a big unit plus leading and
     * trailing pixels so that if a pattern was stitched onto itself, leading
     * pixels connected to trailing pixels, that would make 1 big unit.
     *
     * The size of a pattern is equivalent to one big unit.
     */
    pattern_h = base_unit_px * small_units_per_big_unit;
    int page_h = round_up(height_min, pattern_h);
    int npatterns = page_h/pattern_h;

    /*A page will represent page_size arbitrary unit values, e.g. 70. Those are not pixels*/
    self->page_size = npatterns * big_unit;
//    printf("LadderPageDescriptor page size: %zu, computed from: %d patterns of %d pixels\n", self->page_size, npatterns, pattern_h);

    /* We start the page pretending that we have already etched half a big unit.
     * When the big unit is an odd number of pixels we draw the "bigger half"
     * first and finish the page with the smaller half so that the pages ca be
     * stiched together nicely.
     *
     * Therefore the first big tick mark is at the end of the page minus the
     * half (or big half) the big unit pixel size.
     */
    self->offset = page_h - ceil(pattern_h/2.0);

    self->width = width;
    self->height = page_h;
    self->direction = direction;
    self->init_page = func;


    ruler_core_set_ticks(
       RULER_CORE(self),
       base_unit, base_unit_px,
       base_unit_size, base_unit_labeled,
       nmultiples, multiples
    );

    GenericStrip mockup_strip = {
        .flags = ORIENTATION_VERTICAL|(direction == BOTTUM_UP ? GROW_AGAINST_AXIS : GROW_ALONG_AXIS),
    };

    RULER_CORE(self)->ruler_area = (SDL_Rect){
        .w = width,
        .h = page_h
    };

    //That needs ruler_area to be set before calling ruler_core_set_anchor
    ruler_core_set_anchor(RULER_CORE(self), &mockup_strip, (Anchor){
        .value = 0,
        .position = self->offset
    });

    return self;
}

/**
 * @brief Asks the give LadderPageDescriptor to create a new page. This is the
 * function called by LadderGauge to get new pages. The returned page must be
 * freed by the caller.
 *
 * @param self The LadderPageDescriptor
 * @param index The index of the page to create.
 *
 * @return The newly created LadderPage.
 *
 * @see ladder_page_descriptor_get_idx_for
 */
LadderPage *ladder_page_descriptor_create_page(LadderPageDescriptor *self, int index)
{
    LadderPage *rv;

    rv = ladder_page_new(self, index);
    rv =  self->init_page(rv);

    return rv;
}


/*
 * SPDX-FileCopyrightText: 2021-2026 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "generic-layer.h"
#include "ladder-page.h"
#include "ladder-page-descriptor.h"
#include "generic-strip.h"
#include "ruler-core.h"

/**
 * LadderPage is a base struct that represent a vertical GenericStrip that will
 * be used as a page in a LadderGauge. The LadderGauge displays a tape-like
 * widget that scrolls an infinite slider. The 'infinite' slider is made out of
 * pages.
 *
 * Each page represents a range. All pages making up an infinite slider share a
 * common descriptor that helps understanding the page within the tape and
 * create new ones.
 *
 * Each page can be linked to an index within the strip. Page X can be computed
 * as starting at value V.
 */


/**
 * @brief Creates a new LadderPage with the given descriptor and index. This
 * function is only used by LadderPageDescriptor and should not be called
 * directly by code outside the LadderPageDescriptor machinery. Pages created by
 * ladder_page_new must be freed by the caller using ladder_page_free.
 *
 * The page won't be initialized, the caller must call ladder_page_init to
 * initialize it plus any other function that needs to work on (draw,...) the
 * page.
 *
 * @param descriptor The LadderPageDescriptor that describes the page.
 * @param index The index of the page within the tape.
 *
 * @return A new LadderPage, or NULL if allocation failed.
 *
 * @see ladder_page_free
 */
LadderPage *ladder_page_new(LadderPageDescriptor *descriptor, int index)
{
    LadderPage *self;

    self = calloc(1, sizeof(LadderPage));
    if(self){
        self->descriptor = descriptor;
        self->idx = index;
    }
    return self;
}


/**
 * @brief Initializes a LadderPage: Initializes the canvas and sets the
 * interval. The passed-in LadderPage must have its descriptor and index set.
 *
 * This function should be called by concrete implementations before using the
 * canvas.
 *
 * @param self The LadderPage to initialize.
 *
 * @return The initialized LadderPage, or NULL if initialization failed.
 */
LadderPage *ladder_page_init(LadderPage *self)
{
    GenericStrip *strip;
    GenericLayer *layer;
    LadderPageDescriptor *descriptor;
    int axis_max;

    strip = GENERIC_STRIP(self);
    layer = GENERIC_LAYER(self);
    descriptor = LADDER_PAGE(self)->descriptor;

    bool rv;


    rv = generic_layer_init(GENERIC_LAYER(self), descriptor->width, descriptor->height);
    if(!rv){
        return NULL;
    }

    strip->flags = ORIENTATION_VERTICAL;
    if(descriptor->direction == BOTTUM_UP)
        strip->flags |= GROW_AGAINST_AXIS;
    strip->ppv = RULER_CORE(descriptor)->ticks[0].stride/RULER_CORE(descriptor)->ticks[0].value;
    /*regardless of the orientation, start/end will always be the lower/higher value*/
    axis_max = descriptor->height - 1;

    printf("ladder_page_init(before): first_tick.value: %f, first_tick.position: %d\n", RULER_CORE(descriptor)->first_tick.value, RULER_CORE(descriptor)->first_tick.position);
    ruler_core_set_anchor(RULER_CORE(descriptor), strip, (Anchor){
        .value = self->idx * descriptor->page_size,
        .position = descriptor->offset
    });
    printf("ladder_page_init: page index: %d, first_tick.value: %f first_tick.position: %d\n", self->idx, RULER_CORE(descriptor)->first_tick.value, RULER_CORE(descriptor)->first_tick.position);
    Anchor anchor = RULER_CORE(descriptor)->first_tick;
    if(generic_strip_direction(strip, GROW_ALONG_AXIS)){
        strip->start = anchor.value - anchor.position/strip->ppv;
        strip->end = anchor.value + (axis_max-anchor.position)/strip->ppv;
    }else{ //RulerGrowAgainstAxis
        strip->start = anchor.value - (axis_max-anchor.position)/strip->ppv;
        strip->end = anchor.value + anchor.position/strip->ppv;
    }
//    printf("LadderPageDescriptor ppv is %f\n",strip->ppv);
//    printf("Page marking range is [%f, %f]\n", strip->start, strip->end);
//    printf("Page %d real range is [%f, %f]\n",page_index, strip->start, strip->end);

    return self;
}

/**
 * @brief Frees a LadderPage.
 *
 * @param self The LadderPage to free.
 *
 * @return always NULL (convenience)
 */
void *ladder_page_free(LadderPage *self)
{
    generic_strip_dispose(GENERIC_STRIP(self));
    free(self);
    return NULL;
}

/**
 * @brief Get the pixel increment that matches to a value within the page, i.e.
 * what's the coordinate from that start of the ruler that represents @p value.
 * The value is assumed to be within the range of the page.
 *
 * @param self The LadderPage
 * @param value The value to resolve.
 *
 * @return The pixel increment corresponding to the value.
 */
float ladder_page_resolve_value(LadderPage *self, float value)
{

    ruler_core_set_anchor(RULER_CORE(self->descriptor), GENERIC_STRIP(self), (Anchor){
        .value = self->idx * self->descriptor->page_size,
        .position = self->descriptor->offset
    });

    int rv = ruler_core_get_pixel_increment_for(
        RULER_CORE(self->descriptor),
        GENERIC_STRIP(self), value
    );
//    printf("ladder_page_resolve_value: value: %f, pixel increment: %d, page index: %d, page height: %d\n", value, rv, self->idx, self->descriptor->height);
    return rv;
}

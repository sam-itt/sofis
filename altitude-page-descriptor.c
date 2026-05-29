/*
 * SPDX-FileCopyrightText: 2026 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include "altitude-page-descriptor.h"
#include "misc.h"


AltitudePageDescriptor *altitude_page_descriptor_new(int page_w, int page_h_min,
                                                     uintf8_t unit_px_sz, int small_step_width,
                                                     int big_step_width)
{
    AltitudePageDescriptor *self;
    int pattern_h;

    self = calloc(1, sizeof(AltitudePageDescriptor));
    if(!self)
        return NULL;

    vruler_page_descriptor_init(
        VRULER_PAGE_DESCRIPTOR(self),
        page_w, page_h_min,
        unit_px_sz,
        small_step_width, big_step_width,
        100, 20,
        BOTTUM_UP, altitude_ladder_page_init);

    return self;
}


LadderPage *altitude_ladder_page_init(LadderPage *self)
{
    vruler_ladder_page_init(self);
    generic_layer_build_texture(GENERIC_LAYER(self));

    return self;
}


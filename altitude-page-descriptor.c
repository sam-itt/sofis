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
#include "ladder-page-descriptor.h"
#include "ladder-page.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "misc.h"

AltitudePageDescriptor *altitude_page_descriptor_new(int page_w, int page_h_min)
{
    AltitudePageDescriptor *self;
    int pattern_h;

    uintf8_t unit_px_sz = roundf(page_h_min*0.032f);
    int small_step_width = roundf(0.55f*(page_w*0.15f));
    int big_step_width = roundf(page_w*0.15f);

    self = calloc(1, sizeof(AltitudePageDescriptor));
    if(!self)
        return NULL;

    ladder_page_descriptor_init(
        LADDER_PAGE_DESCRIPTOR(self),
        page_w, page_h_min,
        BOTTUM_UP, 20, unit_px_sz,
        small_step_width, false,
        1, (RulerUnitMultiple[]){
            { .value = 5, .size = big_step_width, .labeled = true }
        },
        altitude_ladder_page_init
    );

    return self;
}


LadderPage *altitude_ladder_page_init(LadderPage *self)
{

    Uint32 white;
    LadderPageDescriptor *descriptor;

    descriptor = LADDER_PAGE_DESCRIPTOR(self->descriptor);

    ladder_page_init(self);
    white = SDL_UWHITE(GENERIC_LAYER(self)->canvas);

    ruler_core_draw_ticks(RULER_CORE(descriptor), GENERIC_STRIP(self),
                          white, true, Left,
                          NULL);
    ruler_core_draw_labels(RULER_CORE(descriptor), GENERIC_STRIP(self),
                           Right, 5,
                           resource_manager_get_font(TERMINUS_16), white, 0,
                           NULL);
    generic_layer_build_texture(GENERIC_LAYER(self));

    return self;
}


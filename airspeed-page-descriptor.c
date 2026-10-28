/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>


#include "airspeed-page-descriptor.h"
#include "generic-layer.h"
#include "ladder-page.h"
#include "ruler-core.h"
#include "sdl-colors.h"

#include "generic-strip.h"
#include "resource-manager.h"

LadderPage *airspeed_ladder_page_init(LadderPage *self);
static void airspeed_ladder_page_draw_arc(LadderPage *self, float start, float end, uintf8_t width, Uint32 color);
static void airspeed_ladder_page_draw_arcs(LadderPage *self);


AirspeedPageDescriptor *airspeed_page_descriptor_new(int page_w, int page_h_min, speed_t v_so,
                                                     speed_t v_s1, speed_t v_fe, speed_t v_no,
                                                     speed_t v_ne)
{
    AirspeedPageDescriptor *self;
    int pattern_h;

    uintf8_t unit_px_sz = roundf(page_h_min*0.0835f);
    int small_step_width = roundf(0.55f*(page_w*0.20f));
    int big_step_width = roundf(page_w*0.20f);


    self = calloc(1, sizeof(AirspeedPageDescriptor));
    if(!self)
        return NULL;

    self->v_so = v_so;
    self->v_s1 = v_s1;
    self->v_fe = v_fe;
    self->v_no = v_no;
    self->v_ne = v_ne;

    self->zones[0] = (ColorZone){
        .from = v_s1, .to = v_no,
        .flags = FromIncluded | ToIncluded,
        .color = SDL_GREEN
    };
    self->zones[1] = (ColorZone){
        .from = v_no, .to = v_ne,
        .flags = FromIncluded | ToIncluded,
        .color = SDL_YELLOW
    };
    self->zones[2] = (ColorZone){
        .from = v_so,
        .to = v_fe,
        .flags = FromIncluded | ToIncluded,
        .color = (SDL_Color){.r=205, .g=205, .b=205, .a=SDL_ALPHA_OPAQUE}
    };
    self->zones[3] = (ColorZone){
        .from = v_ne,
        .to = v_ne+2,
        .flags = FromIncluded | ToIncluded,
        .color = SDL_RED
    };

    ladder_page_descriptor_init(
        LADDER_PAGE_DESCRIPTOR(self),
        page_w, page_h_min,
        BOTTUM_UP, 5, unit_px_sz,
        small_step_width, false,
        1, (RulerUnitMultiple[]){
            { .value = 2, .size = big_step_width, .labeled = true }
        },
        airspeed_ladder_page_init
    );
    ruler_core_preprocess_color_zones(MAX_AIRPSEED_COLOR_ZONES, self->zones);

    return self;
}


LadderPage *airspeed_ladder_page_init(LadderPage *self)
{
    AirspeedPageDescriptor *descriptor;
    Uint32 white;

    descriptor = (AirspeedPageDescriptor *)self->descriptor;

    ladder_page_init(self);
    white = SDL_UWHITE(GENERIC_LAYER(self)->canvas);

    ruler_core_draw_zones(
        RULER_CORE(descriptor), GENERIC_STRIP(self), Right,
        MAX_AIRPSEED_COLOR_ZONES, descriptor->zones, 0.30
    );
    ruler_core_draw_ticks(
        RULER_CORE(descriptor), GENERIC_STRIP(self), white,
        true, Right, NULL
    );
    ruler_core_draw_labels(
        RULER_CORE(descriptor), GENERIC_STRIP(self), Left, 5,
        resource_manager_get_font(TERMINUS_16), white, 0, NULL
    );

    generic_layer_build_texture(GENERIC_LAYER(self));

    return self;
}

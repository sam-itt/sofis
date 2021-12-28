/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include "alt-group.h"
#include "base-gauge.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "SDL_pcf.h"
#include "vertical-stair.h"
#include "res-dirs.h"

static BaseGaugeOps alt_group_ops = {
   .render = (RenderFunc)NULL,
   .update_state = (StateUpdateFunc)NULL,
   .dispose = (DisposeFunc)NULL
};


AltGroup *alt_group_new(void)
{
    AltGroup *self;
    self = calloc(1, sizeof(AltGroup));
    if(self){
        if(!alt_group_init(self)){
            return base_gauge_free(BASE_GAUGE(self));
        }
    }
    return self;
}

AltGroup *alt_group_init(AltGroup *self)
{
    self->altimeter = alt_indicator_new();
    self->vsi = vertical_stair_new(IMG_DIR"/vs-bg.png",IMG_DIR"/vs-cursor.png",
        resource_manager_get_static_font(TERMINUS_16, &SDL_WHITE, 2, PCF_DIGITS, "+-")
    );

    if(!self->vsi || !self->altimeter)
        goto bail;

    /* TODO: Move me as first operation to ensure that Ops are always
     * set when we return NULL so that base_gauge_dispose (called by
     * base_gauge_free) can properly dispose any gauge-allocated resources*/
    base_gauge_init(
        BASE_GAUGE(self),
        &alt_group_ops,
        base_gauge_w(BASE_GAUGE(self->altimeter)) + base_gauge_w(BASE_GAUGE(self->vsi)),
        (base_gauge_h(BASE_GAUGE(self->vsi)) > base_gauge_h(BASE_GAUGE(self->altimeter))) ?
            base_gauge_h(BASE_GAUGE(self->vsi)) :
            base_gauge_h(BASE_GAUGE(self->altimeter))
    );

    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->altimeter), 0, 0);
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->vsi),
        base_gauge_w(BASE_GAUGE(self->altimeter)),
        base_gauge_h(BASE_GAUGE(self->altimeter))/2 - base_gauge_h(BASE_GAUGE(self->vsi))/2
    );
    return self;

bail:
    if(self->vsi) free(self->vsi);
    if(self->altimeter) free(self->altimeter);
    return NULL;
}

void alt_group_set_altitude(AltGroup *self, float value)
{
    alt_indicator_set_value(self->altimeter, value, true);
}

void alt_group_set_vertical_speed(AltGroup *self, float value)
{
    vertical_stair_set_value(self->vsi, value, true);
}

void alt_group_set_values(AltGroup *self, float alt, float vs)
{
    alt_group_set_altitude(self, alt);
    alt_group_set_vertical_speed(self, vs);
}


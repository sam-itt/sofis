/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include "alt-indicator.h"

#include "alt-ladder-page-descriptor.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "misc.h"
#include "tape-gauge.h"

static BaseGaugeOps alt_indicator_ops = {
   .render = (RenderFunc)NULL,
   .update_state = (StateUpdateFunc)NULL,
   .dispose = (DisposeFunc)NULL
};


AltIndicator *alt_indicator_new(void)
{
    AltIndicator *self;
    self = calloc(1, sizeof(AltIndicator));
    if(self){
        if(!alt_indicator_init(self)){
            return base_gauge_free(BASE_GAUGE(self));
        }
    }
    return self;
}

AltIndicator *alt_indicator_init(AltIndicator *self)
{
    PCF_Font *fnt;

    base_gauge_init(BASE_GAUGE(self), &alt_indicator_ops, 68, 240+20);

    /*TODO: Change size to size - 20, when size becomes a parameter !
     * temporary fixed in the rendering function by drawing the ladder first
     * and then drawing on it
     * */
    fnt = resource_manager_get_font(TERMINUS_18);
    DigitBarrel *db = digit_barrel_new(fnt, 0, 9.999, 1);
    DigitBarrel *db2 = digit_barrel_new(fnt, 0, 99, 10);
    self->tape = tape_gauge_new(
        (LadderPageDescriptor*)alt_ladder_page_descriptor_new(),
        AlignRight, 0, 4,
        -1, db2,
        -2, db,
        -2, db,
        -2, db
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->tape), 0, 19);

    self->talt_txt = text_gauge_new(NULL, true, 68, 20);
    self->talt_txt->alignment = HALIGN_CENTER | VALIGN_MIDDLE;
    text_gauge_set_static_font(self->talt_txt,
        resource_manager_get_static_font(TERMINUS_16,
            &SDL_WHITE,
            1, PCF_DIGITS
        )
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->talt_txt), 0, 0);

    text_gauge_set_color(self->talt_txt, SDL_BLACK, BACKGROUND_COLOR);
    text_gauge_set_value(self->talt_txt, "0");

    self->qnh_txt = text_gauge_new(NULL, true, 68, 22);
    self->qnh_txt->alignment = HALIGN_CENTER | VALIGN_MIDDLE;
    text_gauge_set_static_font(self->qnh_txt,
        resource_manager_get_static_font(TERMINUS_16,
            &SDL_WHITE,
            1, PCF_DIGITS
        )
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->qnh_txt),
        0,
        base_gauge_h(BASE_GAUGE(self)) - 20 - 2
    );

    text_gauge_set_color(self->qnh_txt, SDL_BLACK, BACKGROUND_COLOR);
    alt_indicator_set_alt_src(self, ALT_SRC_GPS);

    return self;
}

bool alt_indicator_set_value(AltIndicator *self, float value, bool animated)
{
    return tape_gauge_set_value(self->tape, value, animated);
}

/*HPa*/
void alt_indicator_set_qnh(AltIndicator *self, float value)
{
    int ivalue;

    ivalue = round(value);

    if(self->qnh != ivalue){
        self->qnh = ivalue;
        text_gauge_set_value_formatn(self->qnh_txt,
            4, /*4 digits*/
            "%04d", self->qnh
        );
    }
}

void alt_indicator_set_alt_src(AltIndicator *self, AltSource source)
{
    if(source == ALT_SRC_GPS){
        text_gauge_set_font(self->qnh_txt,
            resource_manager_get_font(TERMINUS_16)
        );
        text_gauge_set_color(self->qnh_txt, SDL_RED, TEXT_COLOR);
        text_gauge_set_value(self->qnh_txt, "GPS");
    }else{
        text_gauge_set_static_font(self->qnh_txt,
            resource_manager_get_static_font(TERMINUS_16,
                &SDL_WHITE,
                1, PCF_DIGITS
            )
        );
        alt_indicator_set_qnh(self, 1013.25);
    }
}

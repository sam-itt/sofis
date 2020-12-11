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
   .update_state = (StateUpdateFunc)NULL
};


AltIndicator *alt_indicator_new(void)
{
    AltIndicator *self;
    self = calloc(1, sizeof(AltIndicator));
    if(self){
        if(!alt_indicator_init(self)){
            free(self);
            return NULL;
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

void alt_indicator_free(AltIndicator *self)
{
    tape_gauge_free(self->tape);
    text_gauge_free(self->talt_txt);
    text_gauge_free(self->qnh_txt);
    base_gauge_dispose(BASE_GAUGE(self));
    free(self);
}

bool alt_indicator_set_value(AltIndicator *self, float value, bool animated)
{
    tape_gauge_set_value(self->tape, value, animated);
}

/*HPa*/
void alt_indicator_set_qnh(AltIndicator *self, float value)
{
    char number[5]; //4 digits, final 0x0
    int ivalue;

    ivalue = round(value);

    if(self->qnh != ivalue){
        self->qnh = ivalue;
        snprintf(number, 5, "%04d", self->qnh);
        text_gauge_set_value(self->qnh_txt, number);
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

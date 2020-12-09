#include <stdio.h>
#include <stdlib.h>

#include "alt-indicator.h"

#include "alt-ladder-page-descriptor.h"
#include "base-gauge.h"
#include "ladder-gauge.h"
#include "odo-gauge.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "SDL_pcf.h"
#include "text-gauge.h"
#include "misc.h"

static void alt_indicator_render(AltIndicator *self, Uint32 dt, RenderContext *ctx);
static void alt_indicator_update_state(AltIndicator *self, Uint32 dt);
static BaseGaugeOps alt_indicator_ops = {
   .render = (RenderFunc)alt_indicator_render,
   .update_state = (StateUpdateFunc)alt_indicator_update_state
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
    self->ladder = ladder_gauge_new((LadderPageDescriptor *)alt_ladder_page_descriptor_new(), -1);
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->ladder), 0, 19);
#if 0
    buffered_gauge_share_buffer(BUFFERED_GAUGE(self->ladder),
        BUFFERED_GAUGE(self),
        0, 19
    );
#endif
    fnt = resource_manager_get_font(TERMINUS_18);
    DigitBarrel *db = digit_barrel_new(fnt, 0, 9.999, 1);
    DigitBarrel *db2 = digit_barrel_new(fnt, 0, 99, 10);
    self->odo = odo_gauge_new_multiple(-1, 4,
            -1, db2,
            -2, db,
            -2, db,
            -2, db
    );
    /*The last -1 in there to prevent eating the border*/
    int odox = (base_gauge_w(BASE_GAUGE(self->ladder)) - base_gauge_w(BASE_GAUGE(self->odo))-1) -1;
    int odoy = 19 + (base_gauge_h(BASE_GAUGE(self->ladder))-1)/2.0 - base_gauge_h(BASE_GAUGE(self->odo))/2.0 +1;
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->odo),
        odox,
        odoy
    );
#if 0
    buffered_gauge_share_buffer(BUFFERED_GAUGE(self->odo),
        BUFFERED_GAUGE(self->ladder),
        (BASE_GAUGE(self->ladder)->w - BASE_GAUGE(self->odo)->w-1) -1,/*The last -1 in there to prevent eating the border*/
        19 + (BASE_GAUGE(self->ladder)->h-1)/2.0 - BASE_GAUGE(self->odo)->h/2.0 +1
    );
#endif
//    buffered_gauge_clear(BUFFERED_GAUGE(self));

    self->talt_txt = text_gauge_new(NULL, true, 68, 20);
    self->talt_txt->alignment = HALIGN_CENTER | VALIGN_MIDDLE;
    text_gauge_set_static_font(self->talt_txt,
        resource_manager_get_static_font(TERMINUS_16,
            &SDL_WHITE,
            1, PCF_DIGITS
        )
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->talt_txt), 0, 0);
#if 0
    buffered_gauge_share_buffer(BUFFERED_GAUGE(self->talt_txt),
        BUFFERED_GAUGE(self),
        0, 0
    );
#endif
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
#if 0
    buffered_gauge_share_buffer(BUFFERED_GAUGE(self->qnh_txt),
        BUFFERED_GAUGE(self),
        0,
        BASE_GAUGE(self)->h - 20 -2
    );
#endif
    text_gauge_set_color(self->qnh_txt, SDL_BLACK, BACKGROUND_COLOR);
    alt_indicator_set_alt_src(self, ALT_SRC_GPS);

    return self;
}

void alt_indicator_free(AltIndicator *self)
{
    ladder_gauge_free(self->ladder);
    odo_gauge_free(self->odo);
    text_gauge_free(self->talt_txt);
    text_gauge_free(self->qnh_txt);
    base_gauge_dispose(BASE_GAUGE(self));
    free(self);
}

bool alt_indicator_set_value(AltIndicator *self, float value, bool animated)
{
    BaseAnimation *animation;
    if(animated){
        if(BASE_GAUGE(self)->nanimations == 0){
            animation = base_animation_new(TYPE_FLOAT, 2,
                &self->ladder->value,
                &self->odo->value
            );
            base_gauge_add_animation(BASE_GAUGE(self), animation);
            base_animation_unref(animation);/*base_gauge takes ownership*/
        }else{
            animation = BASE_GAUGE(self)->animations[0];
        }
        base_animation_start(animation, self->ladder->value, value, DEFAULT_DURATION);
    }else{
        ladder_gauge_set_value(self->ladder, value, false);
        odo_gauge_set_value(self->odo, value, false);
        BASE_GAUGE(self)->dirty = true;
    }
    return true;
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

static void alt_indicator_update_state(AltIndicator *self, Uint32 dt)
{
    BaseAnimation *animation;

    if(BASE_GAUGE(self)->nanimations > 0){
        animation = BASE_GAUGE(self)->animations[0];
        if(!animation->finished){
            BASE_GAUGE(self->ladder)->dirty = true;
            BASE_GAUGE(self->odo)->dirty = true;
        }
    }
}

static void alt_indicator_render(AltIndicator *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_draw_outline(BASE_GAUGE(self), ctx, &SDL_WHITE, NULL);
}


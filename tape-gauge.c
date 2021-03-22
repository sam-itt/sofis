#include <stdarg.h>

#include "base-gauge.h"
#include "sfv-gauge.h"
#include "tape-gauge.h"
#include "ladder-gauge.h"
#include "odo-gauge.h"
#include "misc.h"

static void tape_gauge_update_state(TapeGauge *self, Uint32 dt);
static BaseGaugeOps tape_gauge_ops = {
   .render = (RenderFunc)NULL,
   .update_state = (StateUpdateFunc)tape_gauge_update_state,
   .dispose = (DisposeFunc)NULL
};

TapeGauge *tape_gauge_new(LadderPageDescriptor *descriptor,
                          Alignment align, int xoffset,
                          int nbarrels, ...)
{
    TapeGauge *self, *rv;
    va_list args;

    self = calloc(1, sizeof(TapeGauge));
    if(self){
        va_start(args, nbarrels);
        rv = tape_gauge_vainit(self, descriptor, align, xoffset, nbarrels, args);
        va_end(args);
        if(!rv)
            return base_gauge_free(BASE_GAUGE(self));
    }
    return self;

}

TapeGauge *tape_gauge_vainit(TapeGauge *self, LadderPageDescriptor *descriptor,
                             Alignment align, int xoffset,
                             int nbarrels, va_list ap)
{
    int aligned_x;
    int common_y;
    int max_x;

    self->ladder = ladder_gauge_new((LadderPageDescriptor *)descriptor, -1);
    if(!self->ladder)
        return NULL;
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->ladder), 0, 0);

    self->odo = odo_gauge_vanew_multiple(-1, nbarrels, ap);
    if(!self->odo)
        return NULL;/*self->ladder is a child, base_gauge_free will free it*/

    base_gauge_init(BASE_GAUGE(self),
        &tape_gauge_ops,
        base_gauge_w(BASE_GAUGE(self->ladder)),
        base_gauge_h(BASE_GAUGE(self->ladder))
    );

    common_y = SDLExt_RectMidY(&BASE_GAUGE(self->ladder)->frame)
                   - SDLExt_RectMidY(&BASE_GAUGE(self->odo)->frame);
    /*-1 to prevent from eating the border*/
    max_x = SDLExt_RectLastX(&BASE_GAUGE(self->ladder)->frame) - 1
            - base_gauge_w(BASE_GAUGE(self->odo));
    if(align == AlignRight)
        aligned_x = max_x;
    else /*Left*/
        aligned_x = 0 + 1; /*+1 border, don't eat*/
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->odo),
        clamp(aligned_x + xoffset, 1, max_x), /*1 and not 0, preserve border*/
        common_y
    );

    return self;
}

bool tape_gauge_set_value(TapeGauge *self, float value, bool animated)
{
    BaseAnimation *animation;

    if(animated){
        if(BASE_GAUGE(self)->nanimations == 0){
            animation = base_animation_new(TYPE_FLOAT, 2,
                &SFV_GAUGE(self->ladder)->value,
                &SFV_GAUGE(self->odo)->value
            );
            base_gauge_add_animation(BASE_GAUGE(self), animation);
        }else{
            animation = BASE_GAUGE(self)->animations[0];
        }
        base_animation_start(animation, SFV_GAUGE(self->ladder)->value, value, DEFAULT_DURATION);
    }else{
        ladder_gauge_set_value(self->ladder, value, false);
        odo_gauge_set_value(self->odo, value, false);
        BASE_GAUGE(self)->dirty = true;
    }

    return true;
}

float tape_gauge_get_value(TapeGauge *self)
{
    return sfv_gauge_get_value(SFV_GAUGE(self->ladder));
}

static void tape_gauge_update_state(TapeGauge *self, Uint32 dt)
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

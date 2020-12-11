
/* A base class for all gauges that have a single float value
 * Factor out setting the value with regard to animations and
 * getting the value while animations are running
 *
 * Derivatives are juste supposed to wrap up the setter function
 * if some filtering/clamping is neeeded and let client code use
 * the getter.
 *
 * No virtual functions are implemented, derivatives init/dispose
 * with base_gauge_init/base_gauge_dispose as if they where direct
 * descents of BaseGauge.
 * */

#include "sfv-gauge.h"

/**
 * @brief Sets the value either direcly or triggering a standard
 * duration animation.
 *
 * Intented to be called be derivatives if wrapping is needed or
 * by client code if the derived class doesn't provide a specific
 * setter.
 *
 * @param self a SfvGauge or derivative
 * @param value the value to set
 * @animated true to use an animation from the current value to @p value
 * @returns true on success, false otherwise.
 */
bool sfv_gauge_set_value(SfvGauge *self, float value, bool animated)
{
    bool rv = true;
    BaseAnimation *animation;

//    printf("%s %p value: %f\n",__FUNCTION__, self, self->value);
    if(animated){
        if(BASE_GAUGE(self)->nanimations == 0){
            animation = base_animation_new(TYPE_FLOAT, 1, &self->value);
            base_gauge_add_animation(BASE_GAUGE(self), animation);
            base_animation_unref(animation);/*base_gauge takes ownership*/
        }else{
            animation = BASE_GAUGE(self)->animations[0];
        }
        base_animation_start(animation, self->value, value, DEFAULT_DURATION);
    }else{
        if(value != self->value){
            self->value = value;
            BASE_GAUGE(self)->dirty = true;
        }
    }
    return rv;
}

/**
 * @brief Retreives the value that has been set to the gauge.
 *
 * If an animation is currently running, this function will return
 * the value that has been set to the gauge and not the current frame's
 * intermediate displayed value.
 *
 * @param self a SfvGauge
 * @return the value
 */
float sfv_gauge_get_value(SfvGauge *self)
{
    BaseAnimation *animation;

    if(BASE_GAUGE(self)->nanimations > 0){
        animation = BASE_GAUGE(self)->animations[0];
        if(!animation->finished)
            return animation->end;
    }
    return self->value;
}

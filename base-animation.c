#include <stdio.h>
#include <stdlib.h>

#include "base-animation.h"

static inline void base_animation_process_floats(BaseAnimation *self, float increment);


BaseAnimation *base_animation_new(ValueType type, size_t ntargets, ...)
{
    va_list args;
    BaseAnimation *self, *rv;

    self = calloc(1, sizeof(BaseAnimation));
    if(self){
        va_start(args, ntargets);
        rv = base_animation_vainit(self, type, ntargets, args);
        va_end(args);
        if(!rv){
            free(self);
            return NULL;
        }
    }
    return self;
}

/**
 * @brief Inits a BaseAnimation to animate a set of targets
 *
 * Targets are pointers to the value(s) to animate
 *
 *
 *
 *
 */
BaseAnimation *base_animation_init(BaseAnimation *self, ValueType type, size_t ntargets, ...)
{
    BaseAnimation *rv;
    va_list args;

    va_start(args, ntargets);
    rv = base_animation_vainit(self, type, ntargets, args);
    va_end(args);

    return self;
}

BaseAnimation *base_animation_vainit(BaseAnimation *self, ValueType type, size_t ntargets, va_list ap)
{
    self->ntargets = ntargets;
    self->targets_type = type;
    self->targets = calloc(self->ntargets, sizeof(void*));
    if(!self->targets)
        return NULL;

    for(int i = 0; i < ntargets; i++){
        self->targets[i] = va_arg(ap, void*);
    }
    return self;
}

void base_animation_unref(BaseAnimation *self)
{
    if(self->refcount == 0)
        free(self);
    else
        self->refcount--;
}

void base_animation_ref(BaseAnimation *self)
{
    self->refcount++;
}

void base_animation_start(BaseAnimation *self, float from, float to, float duration)
{
    self->start = from;
    self->end = to;
    self->duration = duration;
    self->time_progress = 0.0;
    self->finished = false;
}

/**
 *
 *
 * @param dt time elapsed since last call (milliseconds)
 * @return value matching the given time
 * */
void base_animation_loop(BaseAnimation *self, uint32_t dt)
{
    float progress;
    float dv;

    self->time_progress += dt; //millis
    progress = self->time_progress / self->duration; /*percentage*/
    if(progress > 1.0f)
        progress = 1.0f;

    dv = self->end - self->start;
    if(self->targets_type == TYPE_FLOAT)
        base_animation_process_floats(self, self->start + dv * progress);
}

static inline void base_animation_process_floats(BaseAnimation *self, float new_value)
{
    for(int i = 0; i < self->ntargets; i++){
        *((float *)self->targets[i]) = new_value;
    }
    if(new_value == self->end)
        self->finished = true;
}

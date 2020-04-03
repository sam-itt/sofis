#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "basic-animation.h"

void basic_animation_start(BasicAnimation *self, float from, float to, float duration)
{
    memset(self, 0, sizeof(BasicAnimation));

    self->start = from;
    self->end = to;
    self->duration = duration;
}

/**
 *
 *
 * @param dt time elapsed since last call (milliseconds)
 * @return value matching the given time
 * */
float basic_animation_loop(BasicAnimation *self, uint32_t dt)
{
    float progress;
    float dv;

    self->time_progress += dt; //millis
    progress = self->time_progress / self->duration; /*percentage*/
    if(progress > 1.0f) 
        progress = 1.0f;

    dv = self->end - self->start;
    self->current = self->start + dv * progress;
    printf("%s returning %f\n",__FUNCTION__, self->current);
    return self->current;
}

#ifndef BASIC_ANIMATION_H
#define BASIC_ANIMATION_H

#include <stdint.h>

typedef struct{
    /*values*/
    float start;
    float end;
    float current;

    /*milliseconds*/
    float duration;
    float time_progress;
}BasicAnimation;


void basic_animation_start(BasicAnimation *self, float from, float to, float duration);
float basic_animation_loop(BasicAnimation *self, uint32_t dt);
#endif /* BASIC_ANIMATION_H */

#ifndef BASE_ANIMATION_H
#define BASE_ANIMATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

/* BaseAnimation can be extended in the future (if needed)
 * by making base_animation_loop virtual there is no need
 * for that at the moment and doing it would add an additional
 * indirection each frame.
 */

typedef enum{
    TYPE_INT8,
    TYPE_UINT8,
    TYPE_INT16,
    TYPE_UINT16,
    TYPE_INT32,
    TYPE_UINT32,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    N_TYPES
}ValueType;

typedef struct{
    /* targets, floats only now
     * will be generic using a void**
     * and a type enum to handle all
     * numeric types.
     * */
    float **targets;
    ValueType targets_type;
    size_t ntargets;

    /*values*/
    float start;
    float end;

    /*milliseconds*/
    float duration;
    float time_progress;

    bool finished;
    bool last_value_reached;
    size_t refcount; /*must be the same type as ntargets*/
}BaseAnimation;

BaseAnimation *base_animation_new(ValueType type, size_t ntargets, ...);
BaseAnimation *base_animation_init(BaseAnimation *self, ValueType type, size_t ntargets, ...);
BaseAnimation *base_animation_vainit(BaseAnimation *self, ValueType type, size_t ntargets, va_list ap);

void base_animation_unref(BaseAnimation *self);
void base_animation_ref(BaseAnimation *self);

void base_animation_start(BaseAnimation *self, float from, float to, float duration);
bool base_animation_loop(BaseAnimation *self, uint32_t dt);
#endif /* BASE_ANIMATION_H */

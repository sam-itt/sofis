#include <stdio.h>
#include <stdlib.h>

#include "alt-group.h"
#include "base-gauge.h"

static void alt_group_render(AltGroup *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);
static BaseGaugeOps alt_group_ops = {
    .render = (RenderFunc)alt_group_render
};

AltGroup *alt_group_new(void)
{
    AltGroup *self;
    self = calloc(1, sizeof(AltGroup));
    if(self){
        if(!alt_group_init(self)){
            free(self);
            return NULL;
        }
    }
    return self;
}

AltGroup *alt_group_init(AltGroup *self)
{
    self->altimeter = alt_indicator_new();
    self->vsi = vertical_stair_new("vs-bg.png","vs-cursor.png", 16);

    if(!self->vsi || !self->altimeter)
        goto bail;

    base_gauge_init(
        BASE_GAUGE(self),
        &alt_group_ops,
        BASE_GAUGE(self->altimeter)->w + BASE_GAUGE(self->vsi)->w,
        (BASE_GAUGE(self->vsi)->h > BASE_GAUGE(self->altimeter)->h) ?
            BASE_GAUGE(self->vsi)->h : BASE_GAUGE(self->altimeter)->h
    );
    return self;

bail:
    if(self->vsi) free(self->vsi);
    if(self->altimeter) free(self->altimeter);
    return NULL;
}

void alt_group_free(AltGroup *self)
{
    alt_indicator_free(self->altimeter);
    vertical_stair_free(self->vsi);
    free(self);
}

void alt_group_set_altitude(AltGroup *self, float value)
{
    alt_indicator_set_value(self->altimeter, value);
}

void alt_group_set_vertical_speed(AltGroup *self, float value)
{
    animated_gauge_set_value(ANIMATED_GAUGE(self->vsi), value);
}

void alt_group_set_values(AltGroup *self, float alt, float vs)
{
    alt_group_set_altitude(self, alt);
    alt_group_set_vertical_speed(self, vs);
}

static void alt_group_render(AltGroup *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location)
{
    SDL_Rect offset = {0,0,0,0};
    offset.x = location->x + BASE_GAUGE(self->altimeter)->w;
    offset.y = location->y +  round(BASE_GAUGE(self->altimeter)->h/2.0) - round((BASE_GAUGE(self->vsi)->h)/2.0);

    base_gauge_render(BASE_GAUGE(self->altimeter), dt, destination, location);
    base_gauge_render(BASE_GAUGE(self->vsi), dt, destination, &offset);
}

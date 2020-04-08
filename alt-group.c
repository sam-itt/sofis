#include <stdio.h>
#include <stdlib.h>

#include "alt-group.h"


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
        return NULL;

    return self;
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

void alt_group_render_at(AltGroup *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location)
{
    SDL_Rect offset = {0,0,0,0};

    SDL_BlitSurface(alt_indicator_render(self->altimeter, dt) , NULL, destination, location);
    offset.x = location->x + alt_indicator_get_width(self->altimeter);
    offset.y = location->y +  round(alt_indicator_get_height(self->altimeter)/2.0) - round((ANIMATED_GAUGE(self->vsi)->view->h)/2.0);
    SDL_BlitSurface(animated_gauge_render(ANIMATED_GAUGE(self->vsi), dt) , NULL, destination, &offset);
}

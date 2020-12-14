#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "base-gauge.h"
#include "scene-node.h"

#define GROW_CHUNK 16;

SceneNode *scene_node_init(SceneNode *self, BaseGauge *gauge)
{
    self->gauge = gauge;
    self->scene_frame = gauge->frame;
    for(BaseGauge *cursor = gauge->parent; cursor != NULL; cursor = cursor->parent){
        /*TODO: rect_offset*/
        self->scene_frame.x += cursor->frame.x;
        self->scene_frame.y += cursor->frame.y;
    }
    gauge->node = self;
    return self;
}

void scene_node_dispose(SceneNode *self)
{
    if(self->above)
        free(self->above);
    if(self->behind)
        free(self->behind);
}

void scene_node_free(SceneNode *self)
{
    scene_node_dispose(self);
    free(self);
}

bool scene_node_add_behind(SceneNode *self, SceneNode *behind)
{
    if(self->nbehind == self->abehind){
        void *tmp;
        self->abehind += GROW_CHUNK;
        tmp = realloc(self->behind, sizeof(SceneNode *)*self->abehind);
        if(!tmp){
            self->abehind -= GROW_CHUNK;
            return false;
        }
        self->behind = tmp;
    }
    self->behind[self->nbehind++] = behind;
    return true;
}

bool scene_node_add_above(SceneNode *self, SceneNode *above)
{
    if(self->nabove == self->aabove){
        void *tmp;
        self->aabove += GROW_CHUNK;
        tmp = realloc(self->above, sizeof(SceneNode *)*self->aabove);
        if(!tmp){
            self->aabove -= GROW_CHUNK;
            return false;
        }
        self->above = tmp;
    }
    self->above[self->nabove++] = above;
    return true;
}

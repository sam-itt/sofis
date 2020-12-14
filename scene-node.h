#ifndef SCENE_NODE_H
#define SCENE_NODE_H
#include <stdbool.h>

#include "SDL_rect.h"

typedef struct _BaseGauge BaseGauge;

typedef struct _SceneNode{
    BaseGauge *gauge;

    SDL_Rect scene_frame; /*In scene coordinates*/
    /*I am behind all these objects, i.e. THEY are above me*/
    struct _SceneNode **behind;
    size_t abehind;
    size_t nbehind;

    /*I am above all these objects, i.e. THEY are behind me*/
    struct _SceneNode **above;
    size_t aabove;
    size_t nabove;
}SceneNode;

SceneNode *scene_node_init(SceneNode *self, BaseGauge *gauge);
void scene_node_dispose(SceneNode *self);
void scene_node_free(SceneNode *self);

bool scene_node_add_behind(SceneNode *self, SceneNode *behind);
bool scene_node_add_above(SceneNode *self, SceneNode *above);
#endif /* SCENE_NODE_H */

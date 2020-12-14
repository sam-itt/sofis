#ifndef RENDERER_H
#define RENDERER_H

#include "base-gauge.h"

typedef struct{
    BaseGauge *gauge;
    SDL_Rect portion;
}RenderEvent;

typedef struct{
    /*TODO: Implement generic array*/
    BaseGauge **gauges;
    size_t agauges; /*allocated*/
    size_t ngauges; /*used*/

    SceneNode *nodes; /*Scene graph*/
    int n_nodes;

    RenderEvent *render_queue;
    size_t aqueue; /*allocated*/
    size_t nqueued; /*used*/

}Renderer;

Renderer *renderer_new(void);
Renderer *renderer_init(Renderer *self);

void renderer_dispose(Renderer *self);
void renderer_free(Renderer *self);

bool renderer_add_gauge(Renderer *self, BaseGauge *gauge, int x, int y);
void renderer_render(Renderer *self, Uint32 dt, RenderContext *ctx);


void renderer_build_scene_graph(Renderer *self);
void renderer_walk_gauge_tree(BaseGauge *gauge, SceneNode *array, size_t curpos, size_t maxpos);

#endif /* RENDERER_H */

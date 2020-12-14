#include <stdio.h>
#include <stdlib.h>

#include "base-gauge.h"
#include "misc.h"
#include "renderer.h"
#include "scene-node.h"

#define GROW_CHUNK 8

Renderer *renderer_new(void)
{
    Renderer *self;

    self = calloc(1, sizeof(Renderer));
    if(self){
        if(!renderer_init(self)){
            free(self);
            return NULL;
        }
    }
    return self;
}

Renderer *renderer_init(Renderer *self)
{
    self->agauges = GROW_CHUNK;
    self->gauges = calloc(self->agauges, sizeof(BaseGauge *));
    if(!self->gauges)
        return NULL;
    return self;
}

void renderer_dispose(Renderer *self)
{
    if(self->gauges){
    /*TODO: Virtual destructor ?*/
#if 0
        for(int i = 0; i < self->ngauges; i++)
            base_gauge_free(self->gauges[i]);
#endif
        free(self->gauges);
    }
    if(self->render_queue)
        free(self->render_queue);
}

void renderer_free(Renderer *self)
{
    renderer_dispose(self);
    free(self);
}

bool renderer_add_gauge(Renderer *self, BaseGauge *gauge, int x, int y)
{
    if(self->ngauges == self->agauges){
        void *tmp;
        self->agauges += GROW_CHUNK;
        tmp = realloc(self->gauges, sizeof(BaseGauge *)*self->agauges);
        if(!tmp){
            self->agauges -= GROW_CHUNK;
            return false;
        }
        self->gauges = tmp;
    }
    self->gauges[self->ngauges] = gauge;
    gauge->frame.x = x;
    gauge->frame.y = y;
    self->ngauges++;

    return true;
}

void renderer_build_scene_graph(Renderer *self)
{
    for(int i = 0; i < self->ngauges; i++){
        self->n_nodes += base_gauge_get_children_count(self->gauges[i], true);
    }

    self->nodes = calloc(self->n_nodes, sizeof(SceneNode));
    for(int i = 0; i < self->ngauges; i++){
        renderer_walk_gauge_tree(self->gauges[i], self->nodes, 0, self->n_nodes);
    }

    self->aqueue = self->n_nodes; /*In fact the worst case is less than that*/
    self->render_queue = calloc(self->aqueue, sizeof(RenderEvent));

    /* Now we have all the nodes in drawing sequence, from first to last
     * with their respective positions
     */
    /*First for all gauges, descend all that are drawn afterwise and
     * register them as behinds (I am behind them), until complete occlusion */
    for(int i = 0; i < self->n_nodes; i++){
        for(int j = i+1; j < self->n_nodes; j++){
            if(SDL_HasIntersection(&self->nodes[i].scene_frame, &self->nodes[j].scene_frame)){
                scene_node_add_behind(&self->nodes[i], &self->nodes[j]);
                if(SDLExt_RectTotalOverlap(&self->nodes[i].scene_frame, &self->nodes[j].scene_frame)){
                    break;
                }
            }

        }
    }
    /* Now in reverse, start at last gauge and go upwards register intersecting gauges
     * as aboves(I am above them, since drawn afterwise) until complete occlusion */
    for(int i = self->n_nodes - 1; i >= 0; i--){
        for(int j = i-1; j >= 0; j--){
            if(SDL_HasIntersection(&self->nodes[i].scene_frame, &self->nodes[j].scene_frame)){
                scene_node_add_above(&self->nodes[i], &self->nodes[j]);
                if(SDLExt_RectTotalOverlap(&self->nodes[i].scene_frame, &self->nodes[j].scene_frame)){
                    break;
                }
            }

        }
    }
}

void renderer_walk_gauge_tree(BaseGauge *gauge, SceneNode *array, size_t curpos, size_t maxpos)
{
    if(curpos == maxpos){
        printf("%s: maxpos reached, shouldn't happen !\n",__FUNCTION__);
        exit(-1);
    }
    scene_node_init(&array[curpos], gauge);
    curpos++;
    for(int i = 0; i < gauge->nchildren; i++){
        renderer_walk_gauge_tree(gauge->children[i], array, curpos, maxpos);
    }
}

void renderer_push_gauge(Renderer *self, BaseGauge *gauge, SDL_Rect *portion)
{
    if(!self->nodes)
        renderer_build_scene_graph(self);

    /* First check if we have to push the gauge. Won't push it if:
     * a) a parent is already scheduled for re-drawing
     * b) the gauge is not visible
     * */
    for(int i = 0; i < self->nqueued; i++){
        if(base_gauge_has_ancestor(gauge, self->render_queue[i].gauge))
            return;
    }

    for(int i = 0; i < gauge->node->nbehind; i++){
        if(SDLExt_RectTotalOverlap(&gauge->node->scene_frame, &gauge->node->behind[i]->scene_frame)){
            return;
        }
    }

    /*I can be redrawn, but I might need to trigger redraws myself*/
    if(gauge->transparent){
        for(int i = 0; i < gauge->node->nabove; i++){
            /*Here the intersecting portion*/
            renderer_push_gauge(self, gauge->node->above[i]->gauge, NULL);
        }
    }
    self->render_queue[self->nqueued].gauge = gauge;
    self->render_queue[self->nqueued].portion = *portion;
}

void renderer_render(Renderer *self, Uint32 dt, RenderContext *ctx)
{
#if USE_SDL_GPU /*When doing 3D/OpenGL just render everything*/
    for(int i = 0; i < self->ngauges; i++){
        SDL_Rect gauge_location = {
            .x = ctx->location->x + self->gauges[i]->frame.x,
            .y = ctx->location->y + self->gauges[i]->frame.y,
            /*The following are only here to prevent distortion when using
             * SDL_Renderer/SDL_Gpu/OpenGL */
            .w = self->gauges[i]->frame.w,
            .h = self->gauges[i]->frame.h,
        };
        RenderContext g_ctx = (RenderContext){
            .target = ctx->target,
            .location = &gauge_location,
            .portion = ctx->portion
        };
        g_ctx.location->x += self->gauges[i]->frame.x;
        g_ctx.location->y += self->gauges[i]->frame.y;
        base_gauge_render(self->gauges[i], dt, &g_ctx);
    }
    return;
#else
    for(int i = 0; i < self->nqueued; i++){
        base_gauge_render(self->render_queue[i].gauge, dt, &(RenderContext){
            .target = ctx->target,
            .location = ctx->location,
            .portion = &self->render_queue[i].portion

        });
    }
#endif
}

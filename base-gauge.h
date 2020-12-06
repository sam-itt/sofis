#ifndef BASE_GAUGE_H
#define BASE_GAUGE_H

#include <SDL2/SDL.h>
#include <SDL_gpu.h>

#include "base-animation.h"
#include "generic-layer.h"

typedef union{
    SDL_Surface *surface;
    GPU_Target* target;
}RenderTarget /*__attribute__ ((__transparent_union__))*/;

typedef void (*RenderFunc)(void *self, Uint32 dt, RenderTarget destination, SDL_Rect *location, SDL_Rect *portion);
typedef void (*StateUpdateFunc)(void *self, Uint32 dt);

typedef struct{
    RenderFunc render;
    StateUpdateFunc update_state;
}BaseGaugeOps;

typedef struct{
    /*width, height and xy position relative to parent*/
    SDL_Rect frame;
}BaseState;

typedef struct _BaseGauge{
    BaseGaugeOps *ops;

    BaseState state; /**/
    bool dirty;

    struct _BaseGauge *parent;

    struct _BaseGauge **children;
    size_t nchildren;
    size_t children_size; /*allocated children*/

    BaseAnimation **animations;
    size_t nanimations;
    size_t animations_size; /*allocated animations*/
}BaseGauge;

#define BASE_GAUGE_OPS(self) ((BaseGaugeOps*)(self))
#define BASE_GAUGE(self) ((BaseGauge *)(self))

#define base_gauge_w(self) ((self)->state.frame.w)
#define base_gauge_h(self) ((self)->state.frame.h)

BaseGauge *base_gauge_init(BaseGauge *self, BaseGaugeOps *ops, int w, int h);
void base_gauge_dispose(BaseGauge *self);

bool base_gauge_add_child(BaseGauge *self, BaseGauge *child, int x, int y);
bool base_gauge_add_animation(BaseGauge *self, BaseAnimation *animation);


void base_gauge_render(BaseGauge *self, Uint32 dt, RenderTarget destination, SDL_Rect *location, SDL_Rect *portion);


int base_gauge_blit_layer(BaseGauge *self, RenderTarget target,
                          SDL_Rect *location, GenericLayer *src,
                          SDL_Rect *srcrect, SDL_Rect *dstrect,
                          SDL_Rect *portion);
int base_gauge_blit_texture(BaseGauge *self,
                            GPU_Target *target, SDL_Rect *location,
                            GPU_Image *src, SDL_Rect *srcrect,
                            SDL_Rect *dstrect, SDL_Rect *portion);
int base_gauge_blit(BaseGauge *self, SDL_Surface *target, SDL_Rect *location,
                     SDL_Surface *src, SDL_Rect *srcrect,
                     SDL_Rect *dstrect, SDL_Rect *portion);

void base_gauge_fill(BaseGauge *self, RenderTarget target, SDL_Rect *location,
                     SDL_Rect *area, void *color, bool packed, SDL_Rect *portion);

void base_gauge_draw_rubis(BaseGauge *self, RenderTarget target, SDL_Rect *location,
                           int y, SDL_Color *color, int pskip, SDL_Rect *portion);

#endif /* BASE_GAUGE_H */

/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef BASE_GAUGE_H
#define BASE_GAUGE_H

#include <SDL2/SDL.h>
#include <SDL_gpu.h>

#include "SDL_pcf.h"
#include "base-animation.h"
#include "generic-layer.h"

typedef union{
    SDL_Surface *surface;
    GPU_Target* target;
}RenderTarget /*__attribute__ ((__transparent_union__))*/;

typedef struct{
    RenderTarget target;
    SDL_Rect *location; /*Location within the target, in target coord space*/

    SDL_Rect *portion; /*Portion of the gauge to render*/
}RenderContext;

typedef void  (*RenderFunc)(void *self, Uint32 dt, RenderContext *ctx);
typedef void  (*StateUpdateFunc)(void *self, Uint32 dt);
typedef void* (*DisposeFunc)(void *self);

typedef struct{
    RenderFunc render;
    StateUpdateFunc update_state;

    DisposeFunc dispose;
}BaseGaugeOps;

typedef struct _BaseGauge{
    BaseGaugeOps *ops;

    /*width, height and xy position relative to parent*/
    SDL_Rect frame;

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

#define base_gauge_w(self) ((self)->frame.w)
#define base_gauge_h(self) ((self)->frame.h)
#define base_gauge_center_y(self) ((base_gauge_h((self))-1)/2)
#define base_gauge_center_x(self) ((base_gauge_w((self))-1)/2)

BaseGauge *base_gauge_init(BaseGauge *self, BaseGaugeOps *ops, int w, int h);
void *base_gauge_dispose(BaseGauge *self);

/**
 * @brief Frees the memory used to store @self and
 * any resources it helds. (internally calls @see
 * base_gauge_dispose)
 *
 * @param self a BaseGauge
 * @return always NULL (convenience)
 */
static inline void *base_gauge_free(BaseGauge *self)
{
    free(base_gauge_dispose(self));
    return NULL;
}

bool base_gauge_add_child(BaseGauge *self, BaseGauge *child, int x, int y);
bool base_gauge_add_animation(BaseGauge *self, BaseAnimation *animation);


void base_gauge_render(BaseGauge *self, Uint32 dt, RenderContext *ctx);

int base_gauge_blit_layer(BaseGauge *self, RenderContext *ctx,
                          GenericLayer *src,
                          SDL_Rect *srcrect, SDL_Rect *dstrect);
int base_gauge_blit_texture(BaseGauge *self, RenderContext *ctx,
                            GPU_Image *src, SDL_Rect *srcrect,
                            SDL_Rect *dstrect);
int base_gauge_blit(BaseGauge *self, RenderContext *ctx,
                    SDL_Surface *src, SDL_Rect *srcrect,
                    SDL_Rect *dstrect);
void base_gauge_fill(BaseGauge *self, RenderContext *ctx,
                     SDL_Rect *area, void *color, bool packed);
void base_gauge_draw_rubis(BaseGauge *self, RenderContext *ctx,
                           int y, SDL_Color *color, int pskip);

void base_gauge_draw_outline(BaseGauge *self, RenderContext *ctx,
                             SDL_Color *color, SDL_Rect *area);

void base_gauge_draw_static_font_patch(BaseGauge *self, RenderContext *ctx,
                                       PCF_StaticFont *font,
                                       PCF_StaticFontPatch *patch);
void base_gauge_draw_static_font_rect_patch(BaseGauge *self, RenderContext *ctx,
                                            PCF_StaticFont *font,
                                            PCF_StaticFontRectPatch *patch);

int base_gauge_blit_rotated_texture(BaseGauge *self, RenderContext *ctx,
                                    GPU_Image *src, SDL_Rect *srcrect,
                                    double angle, SDL_Point *about,
                                    SDL_Rect *dstrect, SDL_Rect *clip);
#endif /* BASE_GAUGE_H */

/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include "SDL_gpu.h"

#include "SDL_rect.h"
#include "base-gauge.h"
#include "sdl-colors.h"
#include "view.h"

#define ALLOC_CHUNK 4
#define ENABLE_SDL_GPU_FUNNY_COORDS 1

BaseGauge *base_gauge_init(BaseGauge *self, BaseGaugeOps *ops, int w, int h)
{
    self->frame.w = w;
    self->frame.h = h;

    self->ops = ops;
    self->dirty = true;

    return self;
}

/**
 * @brief Release all resources held by @param self
 *
 * This function will free any children and animations that @ßelf holds.
 *
 * Subclasses of BaseGauge *can* provide a hook through base_gauge_ops
 * .dispose *if* they need to free specific resources (SDL_Surfaces,
 * char*, ...).
 *
 * If a Subclass only holds references to other BaseGauge-derived types
 * *and* that these have been added as childs with @see base_gauge_add_child
 * they'll be automatically freed and you just need to pass NULL as the
 * dispose op.
 *
 * If you derive a type that have already provided a hook, you are supposed
 * to chain-up using self->super.ops->dispose in your own dispose hook.
 *
 * @param self a BaseGauge
 * @return always self (convenience)
 */
void *base_gauge_dispose(BaseGauge *self)
{
    for(int i = 0; i < self->nchildren; i++)
        base_gauge_free(self->children[i]);

    for(int i = 0; i < self->nanimations; i++){
        base_animation_unref(self->animations[i]);
    }

    if(self->children)
        free(self->children);
    if(self->animations)
        free(self->animations);

    if(self->ops->dispose)
        self->ops->dispose(self);
    return self;
}

/**
 * @brief adds a child
 *
 *
 */
bool base_gauge_add_child(BaseGauge *self, BaseGauge *child, int x, int y)
{
    /* This is not very good... it would be better to avoid dynamic allocation
     * and have that known/fixed on init. Also hurts data locality. Having
     * everything within/aside the struct would be nice */
    if(self->nchildren == self->children_size){
        void *tmp;
        self->children_size += ALLOC_CHUNK;
        tmp = realloc(self->children, sizeof(BaseAnimation*)*self->children_size);
        if(!tmp){
            self->children_size -= ALLOC_CHUNK;
            return false;
        }
        self->children = tmp;
    }
    self->children[self->nchildren] = child;
    child->frame.x = x;
    child->frame.y = y;
    child->parent = self;
    self->nchildren++;

    return true;
}

/**
 * @brief Adds an animation to the gauge.
 *
 * Takes ownership of the animation, you must unref it after calling the function?
 * TODO during refactor: check how that works out
 *
 *
 */
bool base_gauge_add_animation(BaseGauge *self, BaseAnimation *animation)
{
    /* This is not very good... it would be better to avoid dynamic allocation
     * and have that known/fixed on init. Also hurts data locality. Having
     * everything within/aside the struct would be nice */
    if(self->nanimations == self->animations_size){
        void *tmp;
        self->animations_size += ALLOC_CHUNK;
        tmp = realloc(self->animations, sizeof(BaseAnimation*)*self->animations_size);
        if(!tmp){
            self->animations_size -= ALLOC_CHUNK;
            return false;
        }
        self->animations = tmp;
    }
    self->animations[self->nanimations] = animation;
    base_animation_ref(animation);
    self->nanimations++;

    return true;
}


void base_gauge_render(BaseGauge *self, Uint32 dt, RenderContext *ctx)
{
    bool rv;
    for(int i = 0; i < self->nanimations; i++){
        if(!self->animations[i]->finished){
            rv = base_animation_loop(self->animations[i], dt);
            /*If at least one animation has changed something
             * we need to retrace */
            if(rv)
                self->dirty = true;
        }
    }
    if(self->dirty){
        if(self->ops->update_state)
            self->ops->update_state(self, dt);
        self->dirty = false;
    }
    if(self->ops->render)
        self->ops->render(self, dt, ctx);
    for(int i = 0; i < self->nchildren; i++){
        SDL_Rect child_location = {
            .x = ctx->location->x + self->children[i]->frame.x,
            .y = ctx->location->y + self->children[i]->frame.y,
            /*The following are only here to prevent distortion when using
             * SDL_Renderer/SDL_Gpu/OpenGL */
            .w = self->children[i]->frame.w,
            .h = self->children[i]->frame.h,
        };
        /* TODO during refactor: portion will be NULL during the refactor, afterwise compute
         * the correct portion for the child if needed */
        /*TODO refactor: Unroll recursivity*/
        base_gauge_render(self->children[i], dt, &(RenderContext){
            .target = ctx->target,
            .location = &child_location,
            .portion = ctx->portion
        });
    }
}

/*******TAKEN FROM BUFFERED_GAUGE**************/


/**
 * @brief Does a blit from the gauge to @p location using a GenericLayer
 * (encapsulates surface+texture) as source.
 *
 * @param self a RenderTarget
 * @param location The area within the RenderTarget to blit to
 * @param src The GenericLayer to blit from
 * @param srcrect The area of @p src to copy (in @p src coordinate space,
 * i.e 0,0 is @p src top left corner)
 * @param dstrect The area to blit to, in the gauge virtual coordinate
 * space, i.e 0,0 is the gauge top-left corner and will be offseted by
 * location.
 * @param portion The portion of the gauge to blit. NULL for whole
 * @return The underlying lib return value.
 *
 * TODO: INLINE
 */
int base_gauge_blit_layer(BaseGauge *self, RenderContext *ctx,
                          GenericLayer *src,
                          SDL_Rect *srcrect, SDL_Rect *dstrect)
{
#if USE_SDL_GPU
    return base_gauge_blit_texture(self, ctx, src->texture, srcrect, dstrect);
#else
    return base_gauge_blit(self, ctx, src->canvas, srcrect, dstrect);
#endif
}

#define rectf_offset(r1, r2) ((GPU_Rect){(r1)->x + (r2)->x, (r1)->y + (r2)->y, (r1)->w, (r1)->h})
#define rect_offset(r1, r2) ((SDL_Rect){(r1)->x + (r2)->x, (r1)->y + (r2)->y, (r1)->w, (r1)->h})
#define rectf(r) (GPU_Rect){(r)->x, (r)->y, (r)->w, (r)->h}
int base_gauge_blit_texture(BaseGauge *self, RenderContext *ctx,
                            GPU_Image *src, SDL_Rect *srcrect,
                            SDL_Rect *dstrect)
{
    /*TODO: direct GPU_Rect for SDL_gpu*/
    SDL_Rect fdst; /*Final destination*/

    if(dstrect){
        fdst = rect_offset(dstrect, ctx->location);
    }else{
        fdst = (SDL_Rect){
            .x = 0,
            .y = 0,
            .w = base_gauge_w(self),
            .h = base_gauge_h(self)
        };
        fdst = rect_offset(&fdst, ctx->location);
    }

#if 1
//    printf("Before clipping: ");
//    SDLExt_RectDump(&fdst);

    /*Auto clip dest to source to avoid unwanted scaling*/
    if(srcrect){
        fdst.w = fdst.w > srcrect->w ? srcrect->w : fdst.w;
        fdst.h = fdst.h > srcrect->h ? srcrect->h : fdst.h;
    }
//    printf("After clipping: ");
//    SDLExt_RectDump(&fdst);
#endif
    GPU_Rect *dst_rectf, *src_rectf;
    float x,y;

    dst_rectf = &rectf(&fdst);

    if(!srcrect){
        x = src->w/2.0 + dst_rectf->x;
        y = src->h/2.0 + dst_rectf->y;
        src_rectf = NULL;
    }else{
        x = srcrect->w/2.0 + dst_rectf->x;
        y = srcrect->h/2.0 + dst_rectf->y;
        src_rectf = &rectf(srcrect);
    }

    GPU_Blit(src, src_rectf, ctx->target.target, x, y);
    return 0;
}

int base_gauge_blit(BaseGauge *self, RenderContext *ctx,
                     SDL_Surface *src, SDL_Rect *srcrect,
                     SDL_Rect *dstrect)
{
    /*TODO: direct GPU_Rect for SDL_gpu*/
    SDL_Rect fdst; /*Final destination*/

    if(dstrect){
        fdst = rect_offset(dstrect, ctx->location);
    }else{
        fdst = (SDL_Rect){
            .x = 0,
            .y = 0,
            .w = base_gauge_w(self),
            .h = base_gauge_h(self)
        };
        fdst = rect_offset(&fdst, ctx->location);
    }

    return SDL_BlitSurface(src, srcrect, ctx->target.surface, &fdst);
}

void base_gauge_fill(BaseGauge *self, RenderContext *ctx,
                     SDL_Rect *area, void *color, bool packed)
{
    /*TODO: direct GPU_Rect for SDL_gpu*/
    SDL_Rect farea; /*final area*/

    if(area){
        farea = rect_offset(area, ctx->location);
    }else{
        farea = (SDL_Rect){
            .x = 0,
            .y = 0,
            .w = base_gauge_w(self),
            .h = base_gauge_h(self)
        };
        farea = rect_offset(&farea, ctx->location);
    }

#if USE_SDL_GPU
    if(!color)
        return;
    if(packed){
        printf("Packed colors not supported with SDL_gpu\n");
    }

    GPU_RectangleFilled2(ctx->target.target, rectf(&farea), *(SDL_Color*)color);
#else
    if(!color){
        SDL_FillRect(ctx->target.surface, &farea, SDL_UCKEY(ctx->target.surface));
        return;
    }

    if(packed){
        SDL_FillRect(ctx->target.surface, &farea, *((Uint32*)color));
    }else{
        SDL_FillRect(ctx->target.surface,
            &farea,
            SDL_MapRGBA(ctx->target.surface->format,
                ((SDL_Color*)color)->r,
                ((SDL_Color*)color)->g,
                ((SDL_Color*)color)->b,
                ((SDL_Color*)color)->a
            )
        );
    }
#endif
}

void base_gauge_draw_rubis(BaseGauge *self, RenderContext *ctx,
                           int y, SDL_Color *color, int pskip)
{
    SDL_Rect area;

    area = (SDL_Rect){
        .x = 0,
        .y = 0,
        .w = base_gauge_w(self),
        .h = base_gauge_h(self)
    };
    area = rect_offset(&area, ctx->location);

#if USE_SDL_GPU
    int startx, stopx;
    int restartx, endx;
    int liney, half;

    startx = area.x;
    endx = area.x + area.w;
    liney = y + area.y;

    half = round(pskip/2.0);
    stopx = startx + half;
    restartx = endx - half;

    /*
    printf("%s, pushing line from (x:%d,y:%d) to (x:%d,y:%d)\n",__FUNCTION__,startx,liney,stopx,liney);
    printf("%s, pushing line from (x:%d,y:%d) to (x:%d,y:%d)\n",__FUNCTION__,restartx,liney,endx,liney);*/
#if ENABLE_SDL_GPU_FUNNY_COORDS
    /* SDL_GPU has y=0 out of the screen and w,h in the screen
     * like x is [x1, x2[  and y ?
     * */
    liney++;
#endif
    GPU_Line(ctx->target.target, startx, liney, stopx, liney, *color);
    GPU_Line(ctx->target.target, restartx, liney, endx, liney, *color);
#else
    view_draw_rubis(ctx->target.surface, y, color, pskip, &area);
#endif
}

void base_gauge_draw_outline(BaseGauge *self, RenderContext *ctx, SDL_Color *color, SDL_Rect *area)
{
    /*TODO: direct GPU_Rect for SDL_gpu*/
    SDL_Rect farea; /*final area*/

    if(area){
        farea = rect_offset(area, ctx->location);
    }else{
        farea = (SDL_Rect){
            .x = 0,
            .y = 0,
            .w = base_gauge_w(self),
            .h = base_gauge_h(self)
        };
        farea = rect_offset(&farea, ctx->location);
    }
#if USE_SDL_GPU
    /*SDL_GPU treats coordinates as inclusive*/
    farea.x++;
    farea.y++;
    farea.w--;
    farea.h--;

    GPU_Rectangle2(ctx->target.target, rectf(&farea), *color);
#else
    view_draw_outline(ctx->target.surface, color, &farea);
#endif
}

/*TODO: inline without vars*/
void base_gauge_draw_static_font_glyph(BaseGauge *self, RenderContext *ctx,
                                       PCF_StaticFont *font,
                                       SDL_Point *src, SDL_Point *dst)
{
    SDL_Rect dst_rect; /*final destination*/
    SDL_Rect src_rect;

    src_rect = (SDL_Rect){
        .x = src->x,
        .y = src->y,
        .w = font->metrics.characterWidth,
        .h = font->metrics.ascent + font->metrics.descent
    };

    dst_rect = (SDL_Rect){
        .x = dst->x,
        .y = dst->y,
        .w = font->metrics.characterWidth,
        .h = font->metrics.ascent + font->metrics.descent
    };

#if USE_SDL_GPU
    if(!font->texture) /*TODO refactor: Have this a pre-requisite*/
        PCF_StaticFontCreateTexture(font);

    base_gauge_blit_texture(self, ctx, font->texture, &src_rect, &dst_rect);
#else
    base_gauge_blit(self, ctx, font->raster, &src_rect, &dst_rect);
#endif
}

int base_gauge_blit_rotated_texture(BaseGauge *self, RenderContext *ctx,
                                    GPU_Image *src, SDL_Rect *srcrect,
                                    double angle, SDL_Point *about,
                                    SDL_Rect *dstrect, SDL_Rect *clip)
{
    SDL_Rect fdst; /*final destination*/
    SDL_Rect fclip; /*final clip*/

    if(dstrect){
        fdst = rect_offset(dstrect, ctx->location);
    }else{
        fdst = (SDL_Rect){
            .x = 0,
            .y = 0,
            .w = base_gauge_w(self),
            .h = base_gauge_h(self)
        };
        fdst = rect_offset(&fdst, ctx->location);
    }
    if(clip)
        fclip = rect_offset(clip, ctx->location);

    about = about ? about : (srcrect ? &(SDL_Point){.x = srcrect->w/2, .y = srcrect->h/2}
                                     : &(SDL_Point){.x = src->w/2, .y = src->h/2});

	if(!clip){
		GPU_BlitRectX(src,
			srcrect ? &rectf(srcrect) : NULL,
			ctx->target.target,
			&rectf(&fdst),
			angle,
			about->x, about->y,
			GPU_FLIP_NONE
		);
	}else{
		GPU_BlitTransformX(src,
			srcrect ? &rectf(srcrect) : NULL,
			ctx->target.target,
			fclip.x, fclip.y,
			about->x, about->y,
			angle, 1,1
		);
	}
    return 1;
}

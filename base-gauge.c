#include <stdio.h>
#include <stdlib.h>

#include "SDL_gpu.h"
#include "base-animation.h"
#include "base-gauge.h"

#define ALLOC_CHUNK 4

BaseGauge *base_gauge_init(BaseGauge *self, BaseGaugeOps *ops, int w, int h)
{
    self->state.frame.w = w;
    self->state.frame.h = h;



    self->ops = ops;
    self->dirty = true;

    return self;
}

void base_gauge_dispose(BaseGauge *self)
{
    for(int i = 0; i < self->nchildren; i++)
        base_gauge_dispose(self->children[i]);

    for(int i = 0; i < self->nanimations; i++){
        base_animation_unref(self->animations[i]);
    }

    if(self->children)
        free(self->children);
    if(self->animations)
        free(self->animations);
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
    child->state.frame.x = x;
    child->state.frame.y = y;
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


void base_gauge_render(BaseGauge *self, Uint32 dt, RenderTarget destination, SDL_Rect *location, SDL_Rect *portion)
{
    for(int i = 0; i < self->nanimations; i++){
        if(!self->animations[i]->finished){
            base_animation_loop(self->animations[i], dt);
            /*If at least one animation has changed something
             * we need to retrace */
            self->dirty = true;
        }
    }
    //TODO after refactor: update_state is mandatory, provide a standard or remove the test and let fail
    if(self->dirty){
        if(self->ops->update_state)
            self->ops->update_state(self, dt);
        self->dirty = false;
    }
    self->ops->render(self, dt, destination, location, portion);
    for(int i = 0; i < self->nchildren; i++){
        SDL_Rect child_location = {
            .x = location->x + self->children[i]->state.frame.x,
            .y = location->y + self->children[i]->state.frame.x,
            /*The following are only here to prevent distortion when using
             * SDL_Renderer/SDL_Gpu/OpenGL */
            .w = self->children[i]->state.frame.w,
            .h = self->children[i]->state.frame.h,
        };
        /* TODO during refactor: portion will be NULL during the refactor, afterwise compute
         * the correct portion for the child if needed */
        base_gauge_render(self->children[i], dt, destination, &child_location, portion);
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
int base_gauge_blit_layer(BaseGauge *self, RenderTarget target,
                          SDL_Rect *location, GenericLayer *src,
                          SDL_Rect *srcrect, SDL_Rect *dstrect,
                          SDL_Rect *portion)
{
#if USE_SDL_GPU
    return base_gauge_blit_texture(self, target.target, location, src->texture, srcrect, dstrect, portion);
#else
    return base_gauge_blit(self, target.surface, location, src->canvas, srcrect, dstrect, portion);
#endif
}

#define rectf_offset(r1, r2) ((GPU_Rect){(r1)->x + (r2)->x, (r1)->y + (r2)->y, (r1)->w, (r1)->h})
#define rect_offset(r1, r2) ((SDL_Rect){(r1)->x + (r2)->x, (r1)->y + (r2)->y, (r1)->w, (r1)->h})
#define rectf(r) (GPU_Rect){(r)->x, (r)->y, (r)->w, (r)->h}
int base_gauge_blit_texture(BaseGauge *self,
                            GPU_Target *target, SDL_Rect *location,
                            GPU_Image *src, SDL_Rect *srcrect,
                            SDL_Rect *dstrect, SDL_Rect *portion)
{
    SDL_Rect fdst; /*Final destination*/

    if(dstrect){
        fdst = rect_offset(dstrect, location);
    }else{
        fdst = (SDL_Rect){
            .x = 0,
            .y = 0,
            .w = base_gauge_w(self),
            .h = base_gauge_h(self)
        };
        fdst = rect_offset(&fdst, location);
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

    GPU_Blit(src, src_rectf, target, x, y);
    return 0;
}

int base_gauge_blit(BaseGauge *self, SDL_Surface *target, SDL_Rect *location,
                     SDL_Surface *src, SDL_Rect *srcrect,
                     SDL_Rect *dstrect, SDL_Rect *portion)
{
    SDL_Rect fdst; /*Final destination*/

    if(dstrect){
        fdst = rect_offset(dstrect, location);
    }else{
        fdst = (SDL_Rect){
            .x = 0,
            .y = 0,
            .w = base_gauge_w(self),
            .h = base_gauge_h(self)
        };
        fdst = rect_offset(&fdst, location);
    }

    return SDL_BlitSurface(src, srcrect, target, &fdst);
}

void base_gauge_fill(BaseGauge *self, RenderTarget target, SDL_Rect *location, SDL_Rect *area, void *color, bool packed, SDL_Rect *portion)
{
    SDL_Rect farea; /*final area*/

    if(area){
        farea = rect_offset(area, location);
    }else{
        farea = (SDL_Rect){
            .x = 0,
            .y = 0,
            .w = base_gauge_w(self),
            .h = base_gauge_h(self)
        };
        farea = rect_offset(&farea, location);
    }

#if USE_SDL_GPU
    if(!color)
        return;
    if(packed){
        printf("Packed colors not supported with SDL_gpu\n");
    }

    GPU_RectangleFilled2(target.target, rectf(&farea), *(SDL_Color*)color);
#else

    if(!color){
        SDL_FillRect(target.surface, &farea, SDL_UCKEY(target.surface));
        return;
    }

    if(packed){
        SDL_FillRect(target.surface, &farea, *((Uint32*)color));
    }else{
        SDL_FillRect(target.surface,
            &farea,
            SDL_MapRGBA(target.surface->format,
                ((SDL_Color*)color)->r,
                ((SDL_Color*)color)->g,
                ((SDL_Color*)color)->b,
                ((SDL_Color*)color)->a
            )
        );
    }
#endif
}

void base_gauge_draw_rubis(BaseGauge *self, RenderTarget target, SDL_Rect *location,
                           int y, SDL_Color *color, int pskip, SDL_Rect *portion)
{
    SDL_Rect area;

    area = (SDL_Rect){
        .x = 0,
        .y = 0,
        .w = base_gauge_w(self),
        .h = base_gauge_h(self)
    };
    area = rect_offset(&area, location);

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
    GPU_Line(target.target, startx, liney, stopx, liney, *color);
    GPU_Line(target.target, restartx, liney, endx, liney, *color);
#else
    view_draw_rubis(target.surface, y, color, pskip, &area);
#endif
}


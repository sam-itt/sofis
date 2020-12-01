#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "SDL_gpu.h"
#include "SDL_render.h"
#include "misc.h"
#include "render-queue.h"

#define ENABLE_SDL_GPU_FUNNY_COORDS 1

#define is_color_key(c) ((c)->r == 255 && (c)->g == 0 && (c)->b == 255)
//#define is_color_key(c) (0)
#define is_null_rect(r) ((r)->x == -1 && (r)->y == -1 && (r)->w == -1 && (r)->h == -1)
#define rect_offset(r1, r2) ((SDL_Rect){r1->x + r2->x, r1->y + r2->y, r1->w, r1->h})
#define rectf_offset(r1, r2) ((GPU_Rect){r1->x + r2->x, r1->y + r2->y, r1->w, r1->h})
#define rectf(r) (GPU_Rect){(r)->x, (r)->y, (r)->w, (r)->h}


RenderQueue *render_queue_new(size_t size)
{
    RenderQueue *self;

    self = calloc(1, sizeof(RenderQueue));
    if(self){
        if(!render_queue_init(self, size)){
            free(self);
            return NULL;
        }
    }
    return self;
}

RenderQueue *render_queue_init(RenderQueue *self, size_t size)
{
    self->operations = calloc(size, sizeof(RenderOp));
    if(!self->operations)
        return NULL;
    self->allocated_ops = size;

    return self;
}

void render_queue_dispose(RenderQueue *self)
{
    if(self->operations)
        free(self->operations);
}

void render_queue_free(RenderQueue *self)
{
    if(self->refcount == 0){
        render_queue_dispose(self);
        free(self);
    }else{
        self->refcount--;
    }
}

bool render_queue_push_blit(RenderQueue *self, GPU_Image *texture, SDL_Rect *src, SDL_Rect *dst)
{
    if(self->nops >= self->allocated_ops){
        printf("Can't push op, max limit of %d reached\n",self->allocated_ops);
        render_queue_dump(self);
        return false;
    }

    self->operations[self->nops].type = BLIT_OP;
    self->operations[self->nops].blit.texture = texture;
    if(src != NULL)
        self->operations[self->nops].blit.src = *src;
    else
        self->operations[self->nops].blit.src = (SDL_Rect){-1,-1,-1,-1};

    if(dst != NULL)
        self->operations[self->nops].blit.dst = *dst;
    else
        self->operations[self->nops].blit.dst = (SDL_Rect){-1,-1,-1,-1};

    self->nops++;
    return true;
}

bool render_queue_push_outline(RenderQueue *self, SDL_Color *color, SDL_Rect *area)
{
    if(self->nops >= self->allocated_ops){
        printf("Can't push op, max limit of %d reached\n",self->allocated_ops);
        render_queue_dump(self);
        return false;
    }

    self->operations[self->nops].type = OUTLINE_OP;
    self->operations[self->nops].outline.color = *color;
    self->operations[self->nops].outline.rect = *area;

    /*SDL_GPU treats coordinates as inclusive*/
    self->operations[self->nops].outline.rect.x++;
    self->operations[self->nops].outline.rect.y++;
    self->operations[self->nops].outline.rect.w--;
    self->operations[self->nops].outline.rect.h--;

    self->nops++;
    return true;
}

bool render_queue_push_line(RenderQueue *self, SDL_Color *color, int x0, int y0, int x1, int y1)
{
    if(self->nops >= self->allocated_ops){
        printf("Can't push op, max limit of %d reached\n",self->allocated_ops);
        render_queue_dump(self);
        return false;
    }

    self->operations[self->nops].type = LINE_OP;
    self->operations[self->nops].line.color = *color;
    self->operations[self->nops].line.start.x = x0;
    self->operations[self->nops].line.start.y = y0;
    self->operations[self->nops].line.end.x = x1;
    self->operations[self->nops].line.end.y = y1;

#if ENABLE_SDL_GPU_FUNNY_COORDS
    /* SDL_GPU has y=0 out of the screen and w,h in the screen
     * like x is [x1, x2[  and y ?
     * */
    self->operations[self->nops].line.start.y++;
    self->operations[self->nops].line.end.y++;
#endif
    self->nops++;
    return true;
}

bool render_queue_push_clear(RenderQueue *self, SDL_Color *color, SDL_Rect *area)
{
    /*Noop on color key ?*/
    if(!color || is_color_key(color))
        return true;

    if(self->nops >= self->allocated_ops){
        printf("Can't push op, max limit of %d reached\n",self->allocated_ops);
        render_queue_dump(self);
        return false;
    }

    self->operations[self->nops].type = CLEAR_OP;
    if(color)
        self->operations[self->nops].clear.color = *color;
    else
        self->operations[self->nops].clear.color = (SDL_Color){255,0,255};
    if(area){
        self->operations[self->nops].clear.area = *area;
    }
    else
        self->operations[self->nops].clear.area = (SDL_Rect){-1,-1,-1,-1};

    self->nops++;
    return true;
}

bool render_queue_push_rotate(RenderQueue *self, GPU_Image *texture, SDL_Rect *src, SDL_Rect *dst, double angle, SDL_Point *center, SDL_Rect *clip)
{
    if(self->nops >= self->allocated_ops){
        printf("Can't push op, max limit of %d reached\n",self->allocated_ops);
        render_queue_dump(self);
        return false;
    }

    self->operations[self->nops].type = ROTATE_OP;
    self->operations[self->nops].rotate.texture = texture;
    self->operations[self->nops].rotate.angle = angle;
    self->operations[self->nops].rotate.center = *center;

    if(src != NULL)
        self->operations[self->nops].rotate.src = *src;
    else
        self->operations[self->nops].rotate.src = (SDL_Rect){-1,-1,-1,-1};

    if(dst != NULL)
        self->operations[self->nops].rotate.dst = *dst;
    else
        self->operations[self->nops].rotate.dst = (SDL_Rect){-1,-1,-1,-1};
    if(clip != NULL)
        self->operations[self->nops].rotate.clip = *clip;
    else
        self->operations[self->nops].rotate.clip = (SDL_Rect){-1,-1,-1,-1};

    self->nops++;
    return true;
}


static void inline render_clear_op_execute(RenderClearOp *self, GPU_Target *target, SDL_Rect *offset)
{
//    printf("%s\n",__FUNCTION__);
    if(is_color_key(&self->color))
        return;

    if(is_null_rect(&self->area))
        GPU_ClearColor(target, self->color);
    else{
        GPU_RectangleFilled2(target, rectf_offset((&self->area), offset), self->color);
    }
}

void render_clear_op_dump(RenderClearOp *self)
{
    printf("Clear area (x:%d, y:%d, w:%d, h:%d) with color (r: %d, g:%d; b:%d, a:%d)\n",
        self->area.x,self->area.y,self->area.w,self->area.h,
        self->color.r, self->color.g, self->color.b, self->color.a
    );
}

static void inline render_blit_op_execute(RenderBlitOp *self, GPU_Target *target, SDL_Rect *offset)
{
#if 0
    printf("%s texture %p from: ",__FUNCTION__, self->texture);
    SDLExt_RectDump(&self->src);
    printf("To: ");
    SDLExt_RectDump(&rect_offset((&self->dst), offset));
#endif
    GPU_Rect *dst, *src;
    float x,y;

    dst = is_null_rect(&self->dst) ? &(GPU_Rect){0,0,0,0} : &rectf_offset((&self->dst), offset);
    if(is_null_rect(&self->src)){
        x = self->texture->w/2.0 + dst->x;
        y = self->texture->h/2.0 + dst->y;
        src = NULL;
    }else{
        x = self->src.w/2.0 + dst->x;
        y = self->src.h/2.0 + dst->y;
        src = &rectf(&self->src);
    }

    GPU_Blit(self->texture, src, target, x, y);
}





void render_blit_op_dump(RenderBlitOp *self)
{
    printf("Blit texture %p ", self->texture);
    if(is_null_rect(&self->src))
        printf("(whole) ");
    else
        printf(" (area x:%d, y:%d, w:%d, h:%d) ", self->src.x, self->src.y, self->src.w, self->src.h);
    printf("to: ");
    if(is_null_rect(&self->dst))
        printf("(0,0,renderer->width, renderer->height) ");
    else
        printf(" (area x:%d, y:%d, w:%d, h:%d)", self->dst.x, self->dst.y, self->dst.w, self->dst.h);
    printf("\n");
}

static void inline render_line_op_execute(RenderLineOp *self, GPU_Target *target, SDL_Rect *offset)
{
    GPU_Line(target,
        self->start.x + offset->x,
        self->start.y + offset->y,
        self->end.x + offset->x,
        self->end.y + offset->y,
        self->color
    );
}

void render_line_op_dump(RenderLineOp *self)
{
    printf("Draw line from (x:%d,y:%d) to (x:%d,y:%d) using color (r: %d, g:%d; b:%d, a:%d)\n",
        self->start.x,
        self->start.y,
        self->end.x,
        self->end.y,
        self->color.r, self->color.g, self->color.b, self->color.a
    );
}

static void inline render_outline_op_execute(RenderOutlineOp *self, GPU_Target *target, SDL_Rect *offset)
{
//    printf("%s\n",__FUNCTION__);
    if(is_null_rect(&self->rect))
        printf("%s: warning, nullrect as rect\n",__FUNCTION__);
//    if(!is_color_key(&self->color))
    GPU_Rectangle2(target, rectf_offset((&self->rect), offset), self->color);
}

void render_outline_op_dump(RenderOutlineOp *self)
{
    printf("Outline area (x:%d, y:%d, w:%d, h:%d) with color (r: %d, g:%d; b:%d, a:%d)\n",
        self->rect.x, self->rect.y, self->rect.w, self->rect.h,
        self->color.r, self->color.g, self->color.b, self->color.a
    );
}

static void inline render_rotate_op_execute(RenderRotateOp *self, GPU_Target *target, SDL_Rect *offset)
{
	if(is_null_rect(&self->clip)){
		GPU_BlitRectX(self->texture,
			is_null_rect(&self->src) ? NULL : &rectf(&self->src),
			target,
			is_null_rect(&self->dst) ? NULL : &rectf_offset((&self->dst), offset),
			self->angle,
			self->center.x,
			self->center.y,
			GPU_FLIP_NONE
		);
	}else{
		GPU_BlitTransformX(self->texture,
			is_null_rect(&self->src) ? NULL : &rectf(&self->src),
			target,
			self->clip.x, self->clip.y,
			self->center.x,
			self->center.y,
			self->angle, 1,1
		);
	}
}

void render_rotate_op_dump(RenderRotateOp *self)
{
    printf("Rotate texture %p ", self->texture);
    if(is_null_rect(&self->src))
        printf("(whole) ");
    else
        printf(" (area x:%d, y:%d, w:%d, h:%d)", self->src.x, self->src.y, self->src.w, self->src.h);
    printf("%0.2f degrees about (x:%d, y:%d) ", self->angle, self->center.x, self->center.y);
    printf("and blit to: ");
    if(is_null_rect(&self->dst))
        printf("(0,0,renderer->width, renderer->height) ");
    else
        printf(" (area x:%d, y:%d, w:%d, h:%d)", self->dst.x, self->dst.y, self->dst.w, self->dst.h);
    printf("\n");
}
bool render_queue_execute(RenderQueue *self, GPU_Target *target, SDL_Rect *offset)
{
    RenderOp *op;

    offset = offset ? offset : &(SDL_Rect){0,0,0,0};
    self->cleared = false;
//    printf("RenderQueue %p size: %d/%d\n",self,self->nops,self->allocated_ops);

    for(int i = 0; i < self->nops; i++){
        op = &self->operations[i];
        switch(op->type){
        case CLEAR_OP:
            render_clear_op_execute(&op->clear, target, offset);
            break;
        case BLIT_OP:
            render_blit_op_execute(&op->blit, target, offset);
            break;
        case LINE_OP:
            render_line_op_execute(&op->line, target, offset);
            break;
        case OUTLINE_OP:
            render_outline_op_execute(&op->outline, target, offset);
            break;
        case ROTATE_OP:
            render_rotate_op_execute(&op->rotate, target, offset);
            break;
        case NO_OP:
        default:
            break;
        }
    }
    return true;
}

void render_queue_dump(RenderQueue *self)
{

    RenderOp *op;

    for(int i = 0; i < self->nops; i++){
        op = &self->operations[i];
        switch(op->type){
        case CLEAR_OP:
            render_clear_op_dump(&op->clear);
            break;
        case BLIT_OP:
            render_blit_op_dump(&op->blit);
            break;
        case LINE_OP:
            render_line_op_dump(&op->line);
            break;
        case OUTLINE_OP:
            render_outline_op_dump(&op->outline);
            break;
        case ROTATE_OP:
            render_rotate_op_dump(&op->rotate);
        case NO_OP:
        default:
            break;
        }
    }
}
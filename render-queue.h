#ifndef RENDER_QUEUE_H
#define RENDER_QUEUE_H
#include <stdbool.h>
#include <SDL_gpu.h>

#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_stdinc.h"

typedef enum{
    NO_OP = 0,
    CLEAR_OP,
    BLIT_OP,
    LINE_OP,
    OUTLINE_OP,
    ROTATE_OP,

    NUM_OPERATIONS = 5
}OperationType;

typedef struct{
    Uint8 type;
    SDL_Color color;
    SDL_Rect area;
}RenderClearOp;

typedef struct{
    Uint8 type;
    GPU_Image *texture;
    SDL_Rect src;
    SDL_Rect dst;
}RenderBlitOp;

typedef struct{
    Uint8 type;
    SDL_Color color;
    SDL_Point start;
    SDL_Point end;
}RenderLineOp;

typedef struct{
    Uint8 type;
    SDL_Color color;
    SDL_Rect rect;
}RenderOutlineOp;

typedef struct{
    Uint8 type;
    GPU_Image *texture;
    SDL_Rect src;
    SDL_Rect dst;
    double angle;
    SDL_Point center;
    SDL_Rect clip;
}RenderRotateOp;

typedef union {
    Uint8 type;
    RenderClearOp clear;
    RenderBlitOp blit;
    RenderLineOp line;
    RenderOutlineOp outline;
    RenderRotateOp rotate;
}RenderOp;

typedef struct{
    Uint8 refcount;
    bool cleared; /*When sharing a queue avoid chain-clears*/
    RenderOp *operations; /*TODO: Use pointer to pointer AND a pool*/
    size_t nops;
    size_t allocated_ops;
}RenderQueue;


RenderQueue *render_queue_new(size_t size);
RenderQueue *render_queue_init(RenderQueue *self, size_t size);
void render_queue_dispose(RenderQueue *self);
void render_queue_free(RenderQueue *self);

bool render_queue_push_blit(RenderQueue *self, GPU_Image *tex, SDL_Rect *src, SDL_Rect *dst);
bool render_queue_push_outline(RenderQueue *self, SDL_Color *color, SDL_Rect *area);
bool render_queue_push_line(RenderQueue *self, SDL_Color *color, int x0, int y0, int x1, int y1);
bool render_queue_push_clear(RenderQueue *self, SDL_Color *color, SDL_Rect *area);
bool render_queue_push_rotate(RenderQueue *self, GPU_Image *texture, SDL_Rect *src, SDL_Rect *dst, double angle, SDL_Point *center, SDL_Rect *clip);


bool render_queue_execute(RenderQueue *self, GPU_Target *target, SDL_Rect *offset);


void render_queue_dump(RenderQueue *self);
#endif /* RENDER_QUEUE_H */

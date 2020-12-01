#ifndef BUFFERED_GAUGE_H
#define BUFFERED_GAUGE_H
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL_gpu.h>

#include "SDL_render.h"
#include "base-gauge.h"
#include "generic-layer.h"
#include "render-queue.h"
#include "view.h"

typedef enum{
    BUFFER_OWN,
    BUFFER_SHARED
}BufferType;


/* BufferedGauge derivative will draw on their assigned buffer.
 * Therefore there is no need to give them the final on-screen
 * destination raster and area wherein
 */
typedef void (*BufferRenderFunc)(void *self, Uint32 dt);
typedef struct{
    BaseGaugeOps super;

    BufferRenderFunc render;
}BufferedGaugeOps;

#define BUFFERED_GAUGE_OPS(ops) ((BufferedGaugeOps*)(ops))

typedef struct{
    BaseGauge super;

    BufferType type;
    SDL_Point offset;

    SDL_Surface *view;
    bool damaged;

    RenderQueue *queue;
    int max_ops;
}BufferedGauge;

#define BUFFERED_GAUGE(self) ((BufferedGauge*)(self))

#define buffered_gauge_get_view(self) ((self)->view ? (self)->view : buffer_gauge_build_view((self)))
#define buffered_gauge_get_queue(self) ((self)->queue ? (self)->queue : buffer_gauge_build_queue((self)))
#define buffered_gauge_clear(self) buffered_gauge_fill((self), NULL, NULL, false)

#define DEFAULT_QUEUE_SIZE 24

BufferedGauge *buffered_gauge_init(BufferedGauge *self, BufferedGaugeOps *ops, int w, int h);
void buffered_gauge_dispose(BufferedGauge *self);


void buffered_gauge_set_buffer(BufferedGauge *self, SDL_Surface *buffer, int xoffset, int yoffset);
void buffered_gauge_share_buffer(BufferedGauge *self, BufferedGauge *with, int xoffset, int yoffset);

SDL_Surface *buffer_gauge_build_view(BufferedGauge *self);

int buffered_gauge_blit_layer(BufferedGauge *self, GenericLayer *src, SDL_Rect *srcrect, SDL_Rect *dstrect);
int buffered_gauge_blit(BufferedGauge *self, SDL_Surface *src, SDL_Rect *srcrect, SDL_Rect *dstrect);
void buffered_gauge_draw_rubis(BufferedGauge *self, int y, SDL_Color *color, int pskip);
void buffered_gauge_draw_outline(BufferedGauge *self, SDL_Color *color, SDL_Rect *area);


void buffered_gauge_fill(BufferedGauge *self, SDL_Rect *area, void *color, bool packed);

void buffered_gauge_static_font_draw_text(BufferedGauge *self,
                                          SDL_Rect *location,
                                          uint8_t alignment,
                                          const char *string,
                                          PCF_StaticFont *font,
                                          Uint32 bg_color);

void buffered_gauge_font_draw_text(BufferedGauge *self,
                                   SDL_Rect *location,
                                   uint8_t alignment,
                                   const char *string,
                                   PCF_Font *font,
                                   Uint32 text_color,
                                   Uint32 bg_color);



void buffered_gauge_paint_buffer(BufferedGauge *self, Uint32 dt);

void buffered_gauge_render(BufferedGauge *self, Uint32 dt, RenderTarget destination, SDL_Rect *location);

void buffered_gauge_get_area(BufferedGauge *self, SDL_Rect *rect);



RenderQueue *buffer_gauge_build_queue(BufferedGauge *self);
void buffered_gauge_set_render_queue(BufferedGauge *self, RenderQueue *queue, int xoffset, int yoffset);
int buffered_gauge_blit_texture(BufferedGauge *self, GPU_Image *src, SDL_Rect *srcrect, SDL_Rect *dstrect);
int buffered_gauge_blit_rotated_texture(BufferedGauge *self, GPU_Image *src, SDL_Rect *srcrect, double angle, SDL_Point *about, SDL_Rect *dstrect, SDL_Rect *clip);

#endif /* BUFFERED_GAUGE_H */

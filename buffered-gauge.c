#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "base-gauge.h"
#include "buffered-gauge.h"
#include "misc.h"
#include "render-queue.h"
#include "sdl-colors.h"
#include "sdl-pcf/SDL_pcf.h"
#include "vertical-strip.h"
#include "view.h"

/*BufferGauge implements BaseGauge::render and triggers the actual
rendering only if its view have been damaged.*/

void buffered_gauge_render(BufferedGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);
static BufferedGaugeOps buffered_gauge_ops = {
    .super = {
        .render = (RenderFunc)buffered_gauge_render
    },
   .render = NULL /*This is the BufferRenderFunc that derivative will implement*/
};


BufferedGauge *buffered_gauge_init(BufferedGauge *self, BufferedGaugeOps *ops, int w, int h)
{
    ops->super = buffered_gauge_ops.super; /*Take care of the chain-up here so caller only needs to set .render*/
    base_gauge_init(BASE_GAUGE(self), BASE_GAUGE_OPS(ops), w, h);

    self->damaged = true;

    return self;
}

void buffered_gauge_dispose(BufferedGauge *self)
{
    if(self->view)
        SDL_FreeSurface(self->view);
    if(self->queue)
        render_queue_free(self->queue);
}


/**
 * Use the buffer of another gauge for rendering. Useful for combining
 * gauges. Internaly uses either buffered_gauge_set_buffer or
 * buffered_gauge_set_render_queue depending on the compiled rendering mode
 * (using legacy surfaces blit or SDL2 Render API).
 *
 * This is the function you want to use when writing gauges
 *
 * @param with    The 'parent' gauge which buffer you want to use.
 * @param xoffset Position in the surface where the gauge will draw itself.
 * It's like positioning the gauge relative to its parent.
 * @param yoffset see @param xoffset.
 */
void buffered_gauge_share_buffer(BufferedGauge *self, BufferedGauge *with, int xoffset, int yoffset)
{
#if USE_SDL_RENDERER
    buffered_gauge_set_render_queue(self,
        buffered_gauge_get_queue(with),
        xoffset, yoffset
    );
#else
    buffered_gauge_set_buffer(self,
        buffered_gauge_get_view(BUFFERED_GAUGE(with)),
        xoffset, yoffset
    );
#endif
}

/**
 * Enables buffer sharing between gauges: Call buffered_gauge_set_buffer on a
 * newly-created BufferedGauge (before trying to access it's view using
 * buffered_gauge_get_view) and it will use that buffer instead of creating
 * one if its own.
 *
 * Useful for sharing a single buffer when combining gauges
 *
 * @param buffer The surface to be shared. Get it from the other gauge you want
 * to share with using buffered_gauge_get_view.
 * @param xoffset Position in the surface where the gauge will draw itself.
 * It's like positioning the gauge relative to its parent.
 * @param yoffset see @param xoffset.
 */
void buffered_gauge_set_buffer(BufferedGauge *self, SDL_Surface *buffer, int xoffset, int yoffset)
{
    if(self->view)
        SDL_FreeSurface(self->view);

    self->view = buffer;
    buffer->refcount++;
    self->type = BUFFER_SHARED;
    self->offset.x = xoffset;
    self->offset.y = yoffset;
}

/**
 * Enables render-queue sharing between gauges: Call this function on a
 * newly-created BufferedGauge (before trying to access it's queue using
 * buffered_gauge_get_queue) and it will use that queue instead of creating
 * one if its own.
 *
 * Useful for sharing a single render queue when combining gauges
 *
 * @param queue The queue to be shared. Get it from the other gauge you want
 * to share with using buffered_gauge_get_queue.
 * @param xoffset Position in the surface where the gauge will draw itself.
 * It's like positioning the gauge relative to its parent.
 * @param yoffset see @param xoffset.
 */
void buffered_gauge_set_render_queue(BufferedGauge *self, RenderQueue *queue, int xoffset, int yoffset)
{
    if(self->queue)
        render_queue_free(self->queue);

    self->queue = queue;
    queue->refcount++;
    self->type = BUFFER_SHARED;
    self->offset.x = xoffset;
    self->offset.y = yoffset;
}

/*TODO: CHECK IF THESE FUNCTIONS CAN BENEFIT FROM SDL_SetClipRect*/
/* TODO: All of these could be implemented as macros if it weren't for
 * call centralization
 *
 */
/*TODO: Macro/inline?*/
void buffered_gauge_get_area(BufferedGauge *self, SDL_Rect *rect)
{
    rect->x = self->offset.x;
    rect->y = self->offset.y;
    rect->w = BASE_GAUGE(self)->w;
    rect->h = BASE_GAUGE(self)->h;
}

/*TODO: Macro/inline?*/
void buffered_gauge_offset(BufferedGauge *self, SDL_Rect *object, SDL_Rect *result)
{
    result->x = object->x + self->offset.x;
    result->y = object->y + self->offset.y;
    result->w = object->w;
    result->h = object->h;
}

/**
 * Does a blit on the gauge's buffer while taking care of the
 * offset. Avoid having to lookup the view and computing absolute
 * coordinates in gauges drawing code. Also helps centralizing draw
 * calls for future evolutions.
 *
 * @param src The surface to blit on the buffer
 * @param srcrect The portion of src to consider. NULL for whole surface.
 * @param dstrect The location in the buffer. NULL for whole buffer. This
 * destination *must* ignore the buffer's internal offset.
 *
 */
int buffered_gauge_blit(BufferedGauge *self, SDL_Surface *src, SDL_Rect *srcrect, SDL_Rect *dstrect)
{
    SDL_Rect fdst;
    SDL_Surface *tview;

    if(dstrect){
        buffered_gauge_offset(self, dstrect, &fdst);
    }else{
        buffered_gauge_get_area(self, &fdst);
    }

    tview = buffered_gauge_get_view(self);
    return SDL_BlitSurface(src, srcrect, tview, &fdst);
}


int buffered_gauge_blit_texture(BufferedGauge *self, SDL_Texture *src, SDL_Rect *srcrect, SDL_Rect *dstrect)
{
    SDL_Rect fdst;
    RenderQueue *queue;

    if(dstrect){
        buffered_gauge_offset(self, dstrect, &fdst);
    }else{
        buffered_gauge_get_area(self, &fdst);
    }

#if 1
//    printf("Befoire clipping: ");
//    SDLExt_RectDump(&fdst);

    /*Auto clip dest to source to avoid unwanted scaling*/
    if(srcrect){
        fdst.w = fdst.w > srcrect->w ? srcrect->w : fdst.w;
        fdst.h = fdst.h > srcrect->h ? srcrect->h : fdst.h;
    }
//    printf("After clipping: ");
//    SDLExt_RectDump(&fdst);
#endif
    queue = buffered_gauge_get_queue(self);
    return render_queue_push_blit(queue, src, srcrect, &fdst);
}

int buffered_gauge_blit_rotated_texture(BufferedGauge *self, SDL_Texture *src, SDL_Rect *srcrect, double angle, SDL_Point *about, SDL_Rect *dstrect, SDL_Rect *clip)
{
    SDL_Rect fdst;
    RenderQueue *queue;

    if(dstrect){
        buffered_gauge_offset(self, dstrect, &fdst);
    }else{
        buffered_gauge_get_area(self, &fdst);
    }

    queue = buffered_gauge_get_queue(self);
    return render_queue_push_rotate(queue, src, srcrect, &fdst, angle, about, clip);
}


/**
 * Does a blit on the gauge's relative space using a vertical strip
 * as source.
 *
 * @param src The VerticalStrip to blit on the buffer
 * @param srcrect The portion of src to consider. NULL for whole surface.
 * @param dstrect The location in the buffer. NULL for whole buffer. This
 * destination *must* ignore the buffer's internal offset.
 */
int buffered_gauge_blit_strip(BufferedGauge *self, VerticalStrip *src, SDL_Rect *srcrect, SDL_Rect *dstrect)
{
#if USE_SDL_RENDERER
    return buffered_gauge_blit_texture(self, src->rtex, srcrect, dstrect);
#else
    return buffered_gauge_blit(self, src->ruler, srcrect, dstrect);
#endif
}

void buffered_gauge_draw_outline(BufferedGauge *self, SDL_Color *color, SDL_Rect *area)
{
#if USE_SDL_RENDERER
    SDL_Rect farea;
    RenderQueue *queue;

    if(area){
        buffered_gauge_offset(self, area, &farea);
    }else{
        buffered_gauge_get_area(self, &farea);
    }

    queue = buffered_gauge_get_queue(self);
    render_queue_push_outline(queue, color, &farea);
#else
    SDL_Rect farea;
    SDL_Surface *tview;

    if(area){
        buffered_gauge_offset(self, area, &farea);
    }else{
        buffered_gauge_get_area(self, &farea);
    }

    tview = buffered_gauge_get_view(self);
    view_draw_outline(tview, color, &farea);
#endif
}

void buffered_gauge_draw_rubis(BufferedGauge *self, int y, SDL_Color *color, int pskip)
{
    SDL_Rect area;
#if USE_SDL_RENDERER
    RenderQueue *queue;
    int startx, stopx;
    int restartx, endx;
    int liney, half;

    buffered_gauge_get_area(self, &area);
    queue = buffered_gauge_get_queue(self);

    startx = area.x;
    endx = area.x + area.w;
    liney = y + area.y;

    half = round(pskip/2.0);
    stopx = startx + half;
    restartx = endx - half;

    /*
    printf("%s, pushing line from (x:%d,y:%d) to (x:%d,y:%d)\n",__FUNCTION__,startx,liney,stopx,liney);
    printf("%s, pushing line from (x:%d,y:%d) to (x:%d,y:%d)\n",__FUNCTION__,restartx,liney,endx,liney);*/

    render_queue_push_line(queue, color, startx, liney, stopx, liney);
    render_queue_push_line(queue, color, restartx, liney, endx, liney);
#else
    SDL_Surface *tview;

    buffered_gauge_get_area(self, &area);

    tview = buffered_gauge_get_view(self);
    view_draw_rubis(tview, y, color, pskip, &area);
#endif
}

/*TODO: Merge fill/clear*/
void buffered_gauge_fill(BufferedGauge *self, SDL_Rect *area, void *color, bool packed)
{
    SDL_Rect farea;

    if(area){
        buffered_gauge_offset(self, area, &farea);
    }else{
        buffered_gauge_get_area(self, &farea);
    }
#if USE_SDL_RENDERER
    RenderQueue *queue;


    queue = buffered_gauge_get_queue(self);
    render_queue_push_clear(queue, (SDL_Color*)color, &farea);
#else
    SDL_Surface *tview;

    tview = buffered_gauge_get_view(self);
    if(!color){
        SDL_FillRect(tview, &farea, SDL_UCKEY(tview));
        return;
    }

    if(packed){
        SDL_FillRect(tview, &farea, color->packed);
    }else{
        SDL_FillRect(tview,
            &farea,
            SDL_MapRGBA(tview->format,
                color->rgba.r,
                color->rgba.g,
                color->rgba.b,
                color->rgba.a
            )
        );
    }
#endif
}

void buffered_gauge_static_font_draw_text(BufferedGauge *self, SDL_Rect *location,
                                          uint8_t alignment,
                                          const char *string,
                                          PCF_StaticFont *font,
                                          Uint32 bg_color)
{
#if USE_SDL_RENDERER
    SDL_Rect farea;
    RenderQueue *queue;
    SDL_Rect glyph, cursor;
    SDL_Color bg;
    int len;

    if(!font->texture)
        PCF_StaticFontCreateTexture(font, g_renderer);

    if(location){
        buffered_gauge_offset(self, location, &farea);
    }else{
        buffered_gauge_get_area(self, &farea);
    }
    queue = buffered_gauge_get_queue(self);

    if(bg_color != 0xFFFF00FF){
        bg.a = (bg_color >> 24) & 0xff;
        bg.r = (bg_color >> 16) & 0xff;
        bg.g = (bg_color >> 8) & 0xff;
        bg.b = (bg_color) & 0xff;
        render_queue_push_clear(self->queue, &bg, &farea);
    }

    PCF_StaticFontGetSizeRequestRect(font, string, &cursor);
    SDLExt_RectAlign(&cursor, &farea, alignment);
    /*avoid stretching when using SDL_Renderer*/
    cursor.w = font->metrics.characterWidth;

    len = strlen(string);
    for(int i = 0; i < len; i++){
        if(PCF_StaticFontGetCharRect(font, string[i], &glyph) != 0)
            render_queue_push_blit(queue, font->texture, &glyph, &cursor); /*TODO: Move to SDL_pcf ?*/
        cursor.x += font->metrics.characterWidth;
    }
#else
    SDL_Rect farea;
    SDL_Surface *tview;
    SDL_Rect glyph, cursor;
    Uint32 ckey;
    int len;

    if(location){
        buffered_gauge_offset(self, location, &farea);
    }else{
        buffered_gauge_get_area(self, &farea);
    }
    tview = buffered_gauge_get_view(self);

    SDL_GetColorKey(tview, &ckey);
    if(bg_color != ckey)
        SDL_FillRect(tview, &farea, bg_color);

    PCF_StaticFontGetSizeRequestRect(font, string, &cursor);
    SDLExt_RectAlign(&cursor, &farea, alignment);

    len = strlen(string);
    for(int i = 0; i < len; i++){
        //TODO: Remove the test, and draw regardless when default glyph gets implemented
        if(PCF_StaticFontGetCharRect(font, string[i], &glyph)){
            SDL_BlitSurface(font->raster,&glyph,tview, &cursor);

        }
        cursor.x += font->metrics.characterWidth;
    }
#endif
}



void buffered_gauge_font_draw_text(BufferedGauge *self, SDL_Rect *location,
                                   uint8_t alignment,
                                   const char *string, PCF_Font *font,
                                   Uint32 text_color, Uint32 bg_color)
{
#if USE_SDL_RENDERER
    /*TODO: Implement me in SDL_Pcf using DrawPoints*/
    printf("buffered_gauge_font_draw_text not implemeneted using SDL_Renderer, rather use buffered_gauge_static_font_draw_text\n");
#else
    SDL_Rect farea;
    SDL_Surface *tview;

    if(location){
        buffered_gauge_offset(self, location, &farea);
    }else{
        buffered_gauge_get_area(self, &farea);
    }
    tview = buffered_gauge_get_view(self);

    view_font_draw_text(tview, &farea, alignment, string, font, text_color, bg_color);
#endif
}


SDL_Surface *buffer_gauge_build_view(BufferedGauge *self)
{
    if(!self->view){
        self->view = SDL_CreateRGBSurfaceWithFormat(0, BASE_GAUGE(self)->w,  BASE_GAUGE(self)->h, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_SetColorKey(self->view, SDL_TRUE, SDL_UCKEY(self->view));
    }
    return self->view;
}

RenderQueue *buffer_gauge_build_queue(BufferedGauge *self)
{

    if(!self->queue)
        self->queue = render_queue_new(self->max_ops > 0 ? self->max_ops : DEFAULT_QUEUE_SIZE);
    return self->queue;
}

/**
 * Refresh the buffer by asking the underlying object to draw in
 * the buffer
 */
void buffered_gauge_paint_buffer(BufferedGauge *self, Uint32 dt)
{
    BufferedGaugeOps *ops;
    /*TODO: Check if it would be appropriate to clear the buffer here to avoid
     * having clearing code in gauges*/
#if USE_SDL_RENDERER
    if(self->queue && !self->queue->cleared){
        self->queue->nops = 0;
        self->queue->cleared = true; /*When sharing a queue, avoid multiple clears*/
    }
#endif
    ops = BUFFERED_GAUGE_OPS(BASE_GAUGE(self)->ops);
    ops->render(self, dt);
}

void buffered_gauge_render(BufferedGauge *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location)
{
    BufferedGaugeOps *ops;

    if(self->damaged){
        self->damaged = false; /*Set it before so that it can be overrided by the gauge*/
        buffered_gauge_paint_buffer(self, dt);
    }
#if USE_SDL_RENDERER
    render_queue_execute(self->queue, g_renderer, location);
#else
    SDL_BlitSurface(self->view, NULL, destination, location);
#endif
}


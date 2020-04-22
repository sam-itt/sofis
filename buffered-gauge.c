#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL_rect.h"
#include "SDL_surface.h"
#include "base-gauge.h"
#include "buffered-gauge.h"
#include "sdl-colors.h"
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

void buffered_gauge_draw_outline(BufferedGauge *self, SDL_Color *color, SDL_Rect *area)
{
    SDL_Rect farea;
    SDL_Surface *tview;

    if(area){
        buffered_gauge_offset(self, area, &farea);
    }else{
        buffered_gauge_get_area(self, &farea);
    }

    tview = buffered_gauge_get_view(self);
    view_draw_outline(tview, color, &farea);
}

void buffered_gauge_draw_rubis(BufferedGauge *self, int y, SDL_Color *color, int pskip)
{
    SDL_Rect area;
    SDL_Surface *tview;

    buffered_gauge_get_area(self, &area);

    tview = buffered_gauge_get_view(self);
    view_draw_rubis(tview, y, color, pskip, &area);
}

void buffered_gauge_clear(BufferedGauge *self, SDL_Color *color)
{
    SDL_Rect area;
    SDL_Surface *tview;

    buffered_gauge_get_area(self, &area);

    tview = buffered_gauge_get_view(self);
    if(color){
        Uint32 col;

        col = SDL_MapRGBA(tview->format, color->r, color->g, color->b, color->a);
        SDL_FillRect(tview, &area, col);
    }else{
        view_clear(tview, &area);
    }
}

void buffered_gauge_fill(BufferedGauge *self, SDL_Rect *area, SDL_Color *color)
{
    SDL_Rect farea;
    SDL_Surface *tview;
    Uint32 col;

    if(area){
        buffered_gauge_offset(self, area, &farea);
    }else{
        buffered_gauge_get_area(self, &farea);
    }
    tview = buffered_gauge_get_view(self);
    col = SDL_MapRGBA(tview->format, color->r, color->g, color->b, color->a);

    SDL_FillRect(tview, &farea, col);
}

void buffered_gauge_draw_text(BufferedGauge *self, SDL_Rect *location,
                              const char *string, TTF_Font *font,
                              SDL_Color *text_color, SDL_Color *bg_color)
{
    SDL_Rect farea;
    SDL_Surface *tview;


    buffered_gauge_offset(self, location, &farea);
    tview = buffered_gauge_get_view(self);

    view_draw_text(tview, &farea, string, font, text_color, bg_color);
}

SDL_Surface *buffer_gauge_build_view(BufferedGauge *self)
{
    if(!self->view){
        self->view = SDL_CreateRGBSurfaceWithFormat(0, BASE_GAUGE(self)->w,  BASE_GAUGE(self)->h, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_SetColorKey(self->view, SDL_TRUE, SDL_UCKEY(self->view));
    }
    return self->view;
}

/**
 * Refresh the buffer by asking the underlying object to draw in
 * the buffer
 */
void buffered_gauge_paint_buffer(BufferedGauge *self, Uint32 dt)
{
    BufferedGaugeOps *ops;

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
    SDL_BlitSurface(self->view, NULL, destination, location);
}

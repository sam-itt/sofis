#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "SDL_surface.h"
#include "view.h"
#include "sdl-colors.h"

void view_clear(SDL_Surface *self, SDL_Rect *area)
{
    SDL_FillRect(self, area, SDL_UCKEY(self));
}

/**
 * Draws a 1px line around the specified area.
 *
 * @param area The area to encompass with the outline, in self
 * coordinates. This function does not bound-checking: Area MUST be cliped beforehand.
 * If NULL, the whole view will be outlined.
 *
 */
void view_draw_outline(SDL_Surface *self, SDL_Color *rgba, SDL_Rect *area)
{
    int x,y;
    Uint32 *pixels;
    Uint32 color;
    SDL_Surface *canvas;
    int startx,starty;
    int endx, endy;

    canvas = self;
    color = SDL_MapRGBA(self->format, rgba->r, rgba->g, rgba->b, rgba->a);

    /* Warning: end[xy] are not usable coordinates
     * last usable coordinate: end[xy]-1
     */
    if(area){
        startx = area->x;
        endx = area->x + area->w;
        starty = area->y;
        endy = area->y + area->h;
    }else{
        startx = 0;
        endx = self->w,
        starty = 0;
        endy = self->h;
    }

//    printf("startx: %d endx: %d self->w: %d\n",startx,endx,self->w);
//    printf("starty: %d endy: %d self->h: %d\n",starty,endy,self->h);
    assert(startx >= 0 && endx <= self->w);
    assert(starty >= 0 && endy <= self->h);

    SDL_LockSurface(canvas);
    pixels = canvas->pixels;

    /*TODO:
     * -For varying x (drawing top and bottom), do a memset on the range
     * -For varying y (drawing left and right) set both left and right pixels
     *  while doing line y.
     * */
    /*Top line*/
    y = starty;
    for(x = startx; x < endx; x++){
        pixels[y * canvas->w + x] = color;
    }
    /*Bottom line*/
    y = endy - 1;
    for(x = startx; x < endx; x++){
        pixels[y * canvas->w + x] = color;
    }
    /*Left side*/
    x = startx;
    for(y = starty; y < endy; y++){
        pixels[y * canvas->w + x] = color;
    }
    /*Right side*/
    x = endx - 1;
    for(y = starty; y < endy; y++){
        pixels[y * canvas->w + x] = color;
    }

    SDL_UnlockSurface(canvas);
}


void view_draw_text(SDL_Surface *destination, SDL_Rect *location, const char *string, TTF_Font *font, SDL_Color *text_color, SDL_Color *bg_color)
{
    SDL_Surface *text;
    Uint32 bcolor;
    SDL_Rect centered;

    bcolor = SDL_MapRGBA(destination->format, bg_color->r, bg_color->g, bg_color->b, bg_color->a);

    SDL_FillRect(destination, location, bcolor);

    text = TTF_RenderText_Solid(font, string, *text_color);

    /*TODO: Replace this by a centeralized centering macro/function*/
    centered = (SDL_Rect){
        .x = location->x + round(location->w/2.0) - round(text->w/2.0) -1,
        .y = location->y + round(location->h/2.0) - round(text->h/2.0) -1,
        /*Note: w and h are ignored by SDL_BlitSurface*/
        .w = location->w,
        .h = location->h
    };

    SDL_BlitSurface(text, NULL, destination, &centered);
    SDL_FreeSurface(text);
}

/**
 * Draw a "rubis" (the location where to current value
 * aligns on the gauge. Akin to a centerline, but not
 * necessarily located at then center)
 *
 * @param surface the surface to draw on
 * @param y the line (relative to clip, if any, otherwise relative to surface)
 * @param color color of the line
 * @param pskip number of pixels to skip in the middle of the line. 0 for a full uninterrupted
 * line
 * @param clip optional clipping rectangle within surface. If not NULL, the function will consider
 * only the portion of the surface encompassed by clip i.e: if surface is 640x480 and clip is
 * {.x=40,.y=40,.w=20,.h=200} a call to view_draw_rubis with y = 10 will draw a line from
 * {.x=50,.y=50} to {.x=50,.y=69} in surface.
 */
void view_draw_rubis(SDL_Surface *surface, int y, SDL_Color *color, int pskip, SDL_Rect *clip)
{
    Uint32 *pixels;
    Uint32 col;
    int startx, stopx;
    int restartx, endx;
    int liney;

    /* Warning: end[xy] are not usable coordinates
     * last usable coordinate: end[xy]-1
     */
    if(clip){
        startx = clip->x;
        endx = clip->x + clip->w;
        liney = y + clip->y;
    }else{
        startx = 0;
        endx = surface->w;
        liney = y;
    }
    stopx = startx + round(pskip/2.0);
    restartx = endx - stopx;
    col = SDL_MapRGBA(surface->format, color->r, color->g, color->b, color->a);
    SDL_LockSurface(surface);
    pixels = surface->pixels;
    for(int x = startx; x < endx; x++){
        if(!pskip || x < stopx || x >= restartx)
            pixels[liney * surface->w + x] = col;
    }
    SDL_UnlockSurface(surface);
}

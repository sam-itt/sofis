#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "SDL_pixels.h"
#include "SDL_surface.h"
#include "SDL_pcf.h"
#include "sdl-colors.h"
#include "view.h"
#include "misc.h"

/**
 * This file contains generic drawing code that is intended to make
 * static draws directly on SDL_Surfaces. By static we mean that it
 * should be used when we need to generate "textures" that then will
 * be used without changes other than display coordinates on each frame.
 *
 * This is *NOT* intented to be used as per-frame drawing code. All the
 * per-frame drawing code must be in BufferGauge where it can be switched
 * from SDL_Blit to SDL_Renderer, and then to OpenGL.
 */

/**
 * Clear the area using the global color key (SDL_UCKEY)
 *
 */
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

void view_font_draw_text(SDL_Surface *destination, SDL_Rect *location, uint8_t alignment, const char *string, PCF_Font *font, Uint32 text_color, Uint32 bg_color)
{
    SDL_Rect aligned;
    Uint32 ckey;

    location = location ? location : &(SDL_Rect){0, 0, destination->w, destination->h};

    SDL_GetColorKey(destination, &ckey);
    if(bg_color != ckey)
        SDL_FillRect(destination, location, bg_color);

    PCF_FontGetSizeRequestRect(font, string, &aligned);
    SDLExt_RectAlign(&aligned, location, alignment);
    PCF_FontWrite(font, string, text_color, destination, &aligned);
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
    int liney, half;

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

    half = round(pskip/2.0);
    stopx = startx + half;
    restartx = endx - half;

    col = SDL_MapRGBA(surface->format, color->r, color->g, color->b, color->a);
    SDL_LockSurface(surface);
    pixels = surface->pixels;
    for(int x = startx; x < endx; x++){
        if(!pskip || x < stopx || x >= restartx)
            pixels[liney * surface->w + x] = col;
    }
    SDL_UnlockSurface(surface);
}

#include "SDL_render.h"
#include "buffered-gauge.h"
#include "misc.h"
#include "sdl-pcf/SDL_pcf.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "digit-barrel.h"
#include "sdl-colors.h"

DigitBarrel *digit_barrel_new(PCF_Font *font, float start, float end, float step)
{
    DigitBarrel *self;

    self = calloc(1, sizeof(DigitBarrel));
    if(self){
        if(!digit_barrel_init(self, font, start, end, step)){
            free(self);
            return NULL;
        }
    }
//    printf("DigitBarrel %p new\n",self);
    return self;
}

/**
 * Creates an odometer-like gauge from @param start to @param end
 *
 * @param font_size: Font size to use in pixels
 * @param start: first value (i.e 0, 10, etc.)
 * @param end: last "full" value. Should be 9.999, 99, etc. To account for rotation
 * @param step: increment between two values (1,15, etc.)
 */
DigitBarrel *digit_barrel_init(DigitBarrel *self, PCF_Font *font, float start, float end, float step)
{
    Uint32 advance, font_height;
    int vheight, vwidth;
    int ndigits;
    float maxvalue;
    float minv, maxv;
    SDL_Surface *tmp;
    char *number;
    SDL_Rect cursor; /*write cursor (into VERTICAL_SPLIT(self)->ruler)*/
    SDL_Rect fcursor; /*font read cursor*/
    char fmt[10];
    VerticalStrip *strip;

    strip = VERTICAL_STRIP(self);

    strip->start = start;
    strip->end = end;

    maxvalue = fabs((end > start) ? end : start);
    maxvalue = (maxvalue > 0) ? maxvalue : 1;
    ndigits = floor(log10f(maxvalue) + 1);

    number = alloca(sizeof(char)*(ndigits+1)); /*There shouldn't be that much digits*/
    snprintf(fmt, 10, "%%0%dd", ndigits);

    PCF_FontGetSizeRequest(font, "0", &advance, &font_height);
    vheight = round((fabs(end-start)) / step)*font_height; /*end should be .9999 so there is no need for the +1*/
    vwidth = advance * ndigits;
    strip->ruler = SDL_CreateRGBSurface(0, vwidth, vheight, 32, 0, 0, 0, 0);

    minv = (start < end) ? start : end;
    maxv = (start > end) ? start : end;
    cursor = (SDL_Rect){0,0,strip->ruler->w, font_height};
    fcursor = (SDL_Rect){0,0,vwidth, font_height};
    Uint32 white = SDL_UWHITE(strip->ruler);
    for(float i = minv; i < maxv; i += step){
        snprintf(number, 6, fmt, (int)round(i));
        PCF_FontWrite(font, number, white, strip->ruler, &cursor);
        cursor.y += cursor.h;
        cursor.x = 0; /*PCF_FontWrite advances the cursor*/
    }

    self->symbol_h = font_height;
    self->fei = round((self->symbol_h-1)/2.0);
    /* The value is the centerline of the digit.
     * There is exactly @param step between two digits'
     * centerlines. This dimension (between centerlnes)
     * happend to be the same as the symbol size,i as it
     * is two consective halves of a symbol
     * */
    strip->ppv = self->symbol_h / step;

//    digit_barrel_draw_etch_marks(self);
#if USE_SDL_RENDERER
    strip->rtex = SDL_CreateTextureFromSurface(g_renderer, strip->ruler); /*TODO: This must go a layer up (or down)*/
#endif
    return self;
}

void digit_barrel_free(DigitBarrel *self)
{
    if(self->refcount > 0)
        self->refcount--;
    if(!self->refcount){
//        printf("DigitBarrel %p free\n",self);
        vertical_strip_dispose(VERTICAL_STRIP(self));
        free(self);
    }
}




/**
 * Returns pixel index (i.e y)
 * from a given value
 *
 * @param value the value to look for
 * @param reverse if true, assumes that 0 is a the bottom of the strip instead of
 * at the top
 * @return the pixel index in {@link #ruler} that maps to @param value, or -1 if value
 * can't be mapped (outside of the [{@link #start}, {@link #end}] range)
 */
float digit_barrel_resolve_value(DigitBarrel *self, float value)
{
    VerticalStrip *strip;
    float y;

    strip = VERTICAL_STRIP(self);
    if(!vertical_strip_has_value(strip, value))
        return -1;

    value = fmod(value, fabs(strip->end - strip->start) + 1);
    y =  value * strip->ppv + self->fei;

    return round(y);
}


/*
 * Rubis is the offset in region where to align the value fomr the spinner.
 * if its negative, the rubis will be (vertical) the center dst: the y index
 * in the value spinner representing @param value will be aligned with the middle
 *
 */
void digit_barrel_render_value(DigitBarrel *self, float value, BufferedGauge *dst, SDL_Rect *region, float rubis)
{
    float y;
    SDL_Rect dst_region = {region->x,region->y,region->w,region->h};
    VerticalStrip *strip;
    strip = VERTICAL_STRIP(self);

    /*translate @param value to an index in the spinner texture*/
    y = digit_barrel_resolve_value(self, value);
    rubis = (rubis < 0) ? region->h / 2.0 : rubis;
    SDL_Rect portion = {
        .x = 0,
        .y = round(y - rubis),
        .w = strip->ruler->w,
        .h = region->h
    };
    /* Ensures that portion.y + portion.h doesn't got past image bounds:
     * w/h are ignored by SDL_BlitSurface, but when using SDL_Renderers wrong
     * values will stretch the image.
     */
    if(portion.y + portion.h > strip->ruler->h)
        portion.h = strip->ruler->h - portion.y;


    if(portion.y < 0){ //Fill top
        SDL_Rect patch = {
            .x = 0,
            .y = strip->ruler->h + portion.y, //means - portion.y as portion.y < 0 here
            .w = strip->ruler->w,
            .h = strip->ruler->h - patch.y
        };
#if USE_SDL_RENDERER
        buffered_gauge_blit_texture(dst, strip->rtex, &patch, &dst_region);
#else
        buffered_gauge_blit(dst, strip->ruler, &patch, &dst_region);
#endif
        dst_region.y = patch.h;
        portion.y = 0;
        portion.h -= patch.h;
    }
#if USE_SDL_RENDERER
    buffered_gauge_blit_texture(dst, strip->rtex, &portion, &dst_region);
#else
    buffered_gauge_blit(dst, strip->ruler, &portion, &dst_region);
#endif
    if(portion.y + region->h > strip->ruler->h){// fill bottom
        float taken = strip->ruler->h - portion.y; //number of pixels taken from the bottom of values pict
        float delta = region->h - taken;
        dst_region.y += taken;
        SDL_Rect patch = {
            .x = 0,
            .y = 0,
            .w = strip->ruler->w,
            .h = delta
        };
#if USE_SDL_RENDERER
        buffered_gauge_blit_texture(dst, strip->rtex, &patch, &dst_region);
#else
        buffered_gauge_blit(dst, strip->ruler, &patch, &dst_region);
#endif
    }
}

void digit_barrel_draw_etch_marks(DigitBarrel *self)
{
    int iy;
    VerticalStrip *strip;

    strip = VERTICAL_STRIP(self);

    SDL_LockSurface(strip->ruler);

    Uint32 *pixels = strip->ruler->pixels;
    Uint32 color = SDL_UYELLOW(strip->ruler);
    float count = 0;
    for(float y = self->fei; round(y) < strip->ruler->h; y += self->symbol_h/2.0){
        iy = round(y);
//        printf("%0.2f y = %d\n",count,iy);
        for(int x = 0; x < strip->ruler->w; x++){
            pixels[iy * strip->ruler->w + x] = color;
        }
        count += 5.0;
    }
    SDL_UnlockSurface(strip->ruler);
}



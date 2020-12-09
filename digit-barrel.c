#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "digit-barrel.h"
#include "sdl-colors.h"
#include "misc.h"


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
    GenericLayer *layer;

    strip = VERTICAL_STRIP(self);
    layer = GENERIC_LAYER(self);

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

    generic_layer_init_with_masks(GENERIC_LAYER(self), vwidth, vheight, 0, 0, 0, 0);

    minv = (start < end) ? start : end;
    maxv = (start > end) ? start : end;
    cursor = (SDL_Rect){0,0,generic_layer_w(layer), font_height};
    fcursor = (SDL_Rect){0,0,vwidth, font_height};
    Uint32 white = SDL_UWHITE(layer->canvas);
    for(float i = minv; i < maxv; i += step){
        snprintf(number, 6, fmt, (int)round(i));
        PCF_FontWrite(font, number, white, layer->canvas, &cursor);
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

    generic_layer_build_texture(layer);
//    digit_barrel_draw_etch_marks(self);
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
void digit_barrel_state_value(DigitBarrel *self, float value, SDL_Rect *region, float rubis, DigitBarrelState *state)
{
    float y;
    SDL_Rect dst_region = {region->x,region->y,region->w,region->h};
    VerticalStrip *strip;
    GenericLayer *layer;

    strip = VERTICAL_STRIP(self);
    layer = GENERIC_LAYER(self);

    /*translate @param value to an index in the spinner texture*/
    y = digit_barrel_resolve_value(self, value);
    rubis = (rubis < 0) ? region->h / 2.0 : rubis;
    SDL_Rect portion = {
        .x = 0,
        .y = round(y - rubis),
        .w = generic_layer_w(layer),
        .h = region->h
    };
    /* Ensures that portion.y + portion.h doesn't got past image bounds:
     * w/h are ignored by SDL_BlitSurface, but when using SDL_Renderers wrong
     * values will stretch the image.
     */
    if(portion.y + portion.h > generic_layer_h(layer))
        portion.h = generic_layer_h(layer) - portion.y;

    state->layer = layer;
    if(portion.y < 0){ //Fill top
        SDL_Rect patch = {
            .x = 0,
            .y = generic_layer_h(layer) + portion.y, //means - portion.y as portion.y < 0 here
            .w = generic_layer_w(layer),
            .h = generic_layer_h(layer) - patch.y
        };
        state->patches[state->npatches].src = patch;
        state->patches[state->npatches].dst = dst_region;
        state->npatches++;

        dst_region.y = patch.h;
        portion.y = 0;
        portion.h -= patch.h;
    }
    state->patches[state->npatches].src = portion;
    state->patches[state->npatches].dst = dst_region;
    state->npatches++;

    if(portion.y + region->h > generic_layer_h(layer)){// fill bottom
        float taken = generic_layer_h(layer) - portion.y; //number of pixels taken from the bottom of values pict
        float delta = region->h - taken;
        dst_region.y += taken;
        SDL_Rect patch = {
            .x = 0,
            .y = 0,
            .w = generic_layer_w(layer),
            .h = delta
        };
        state->patches[state->npatches].src = patch;
        state->patches[state->npatches].dst = dst_region;
        state->npatches++;
    }
}


void digit_barrel_draw_etch_marks(DigitBarrel *self)
{
    int iy;
    VerticalStrip *strip;
    GenericLayer *layer;

    strip = VERTICAL_STRIP(self);
    layer = GENERIC_LAYER(self);

    generic_layer_lock(layer);

    Uint32 *pixels = layer->canvas->pixels;
    Uint32 color = SDL_UYELLOW(layer->canvas);
    float count = 0;
    for(float y = self->fei; round(y) < generic_layer_h(layer); y += self->symbol_h/2.0){
        iy = round(y);
//        printf("%0.2f y = %d\n",count,iy);
        for(int x = 0; x < generic_layer_w(layer); x++){
            pixels[iy * generic_layer_w(layer) + x] = color;
        }
        count += 5.0;
    }
    generic_layer_unlock(layer);
}



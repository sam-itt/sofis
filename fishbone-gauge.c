/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL_gpu.h>

#include "SDL_rect.h"
#include "base-gauge.h"
#include "fishbone-gauge.h"

#include "generic-ruler.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "SDL_pcf.h"
#include "misc.h"
#include "res-dirs.h"

#define view_set_pixel(surface, x, y, color) (Uint32 *)((surface)->pixels)[(y)*(surface)->width+(x)] = (color)

static void fishbone_gauge_render(FishboneGauge *self, Uint32 dt, RenderContext *ctx);
static void fishbone_gauge_update_state(FishboneGauge *self, Uint32 dt);
static void *fishbone_gauge_dispose(FishboneGauge *self);
static BaseGaugeOps fishbone_gauge_ops = {
   .render = (RenderFunc)fishbone_gauge_render,
   .update_state = (StateUpdateFunc)fishbone_gauge_update_state,
   .dispose = (DisposeFunc)fishbone_gauge_dispose
};


/**
 * @brief Creates a new FishboneGauge. Calling code is responsible
 * for the freeing.
 *
 * @see fishbone_gauge_init for params and return value
 */
FishboneGauge *fishbone_gauge_new(bool marked,
                                  PCF_Font *font, SDL_Color color,
                                  float from, float to, float step,
                                  int bar_max_w, int bar_max_h,
                                  int nzones, ColorZone *zones)
{
    FishboneGauge *self;
    bool rv;

    self = calloc(1, sizeof(FishboneGauge));
    if(self){
        rv = fishbone_gauge_init(self, marked,
            font, color,
            from, to, step,
            bar_max_w, bar_max_h,
            nzones, zones);
        if(!rv){
            return base_gauge_dispose(BASE_GAUGE(self));
        }
    }
    return self;
}

/**
 * @brief Inits a FishboneGauge
 *
 * @param self a FishboneGauge
 * @param marked Write values corresponding to hatch marks
 * @param font The font to use for markings
 * @param color the color to use when writing hatch marks and markings
 * @param from start of the value range
 * @param to end of the value range
 * @param step increment in value units (meters, degrees, etc.) of hatch
 * marks. Passing in a negative value will create only hatch marks at the
 * begining/end of the range. See generic_ruler_init for a discussion of
 * valid @p step values.
 * @param bar_max_w Maximum width of the bar itself. It can be shrinked to
 * accomodate the needed number of hatch marks.
 * @param bar_max_h Maximum height of the bar itself. The gauge will be
 * larger to accomadate markings (if any) and cursor(s).
 * @param nzones size of the @p zones array, 0 if none
 * @param zones array of ColorZones that will be used in the gauge. NULL
 * for none.
 * @return @p self on success, NULL on failure.
 *
 * @note Array is copied, caller can pass in a temporary local array.
 *
 * @see generic_ruler_init
 * @see fishbone_gauge_new
 */
FishboneGauge *fishbone_gauge_init(FishboneGauge *self,
                                   bool marked,
                                   PCF_Font *font, SDL_Color color,
                                   float from, float to, float step,
                                   int bar_max_w, int bar_max_h,
                                   int nzones, ColorZone *zones)
{

    generic_ruler_init(&(self->ruler),
        RulerHorizontal, RulerGrowAlongAxis,
        from, to, step,
        font, Bottom, 0,
        bar_max_w, bar_max_h
    );

    self->nzones = nzones;
    self->zones = calloc(self->nzones, sizeof(ColorZone));
    if(!self->zones)
        return NULL;

    for(int i = 0; i < nzones; i++){
        self->zones[i] = zones[i];
    }

    Uint32 fcolor;
    fcolor =  SDL_MapRGBA(
        GENERIC_LAYER(&self->ruler)->canvas->format,
        color.r, color.g, color.b, color.a
    );

    /*Draws the ruler*/
    if(self->nzones > 0){
        generic_ruler_draw_zones(&self->ruler, Center, self->nzones, self->zones, 0.7);
    }
    generic_ruler_etch_hatches(&(self->ruler), fcolor, false, true, Center);
    if(marked && font) /*Font will also be used to tag the cursors (itf)*/
        generic_ruler_etch_markings(&(self->ruler), Bottom, font, fcolor, 0);
    generic_layer_build_texture(GENERIC_LAYER(&self->ruler));

    /* Loads the cursor.
     * TODO: Support 2 cursors and generate them otf
     * to match the font size. Have the ability to enclose
     * a letter within the triangle.
     */
    self->cursor = generic_layer_new_from_file(IMG_DIR"/fishbone-cursor.png");
    if(!self->cursor)
        return NULL;
    generic_layer_build_texture(self->cursor);

    int extra_h = SDLExt_RectMidY(&self->ruler.ruler_area) - self->cursor->canvas->w;
    /*Does Cursor go out of the area?*/
    extra_h = (extra_h < 0) ? abs(extra_h) : 0;
    /* Check if the cursor goes outside of the ruler area when at it's leftmost
     * position.
     * TODO: Support GrowAgainstAxis*/
    int first_etch = generic_ruler_get_pixel_increment_for(&self->ruler, self->ruler.start);
    int delta = self->ruler.ruler_area.x - first_etch - generic_layer_w(self->cursor)/2;
    delta = (delta < 0) ? abs(delta) : 0;
    self->ruler_rect = (SDL_Rect){
        .x = delta,
        .y = extra_h,
        .w = GENERIC_LAYER(&self->ruler)->canvas->w,
        .h = GENERIC_LAYER(&self->ruler)->canvas->h
    };
    self->state.cursor_rect = (SDL_Rect){
        .x = 0,
        .y = 0,
        .w = self->cursor->canvas->w,
        .h = self->cursor->canvas->h
    };

    base_gauge_init(BASE_GAUGE(self),
        &fishbone_gauge_ops,
        GENERIC_LAYER(&self->ruler)->canvas->w,
        GENERIC_LAYER(&self->ruler)->canvas->h + extra_h
    );

    return self;
}

/**
 * @brief Release resources held by @p self.
 *
 * @param self a FishboneGauge
 */
static void *fishbone_gauge_dispose(FishboneGauge *self)
{
    generic_ruler_dispose(&self->ruler);
    if(self->cursor)
        generic_layer_free(self->cursor);
    if(self->zones)
        free(self->zones);

    return self;
}

bool fishbone_gauge_set_value(FishboneGauge *self, float value, bool animated)
{
    bool rv = true;
    BaseAnimation *animation;

    generic_ruler_clip_value(&self->ruler, &value);

    return sfv_gauge_set_value(SFV_GAUGE(self), value, animated);
}

static void fishbone_gauge_update_state(FishboneGauge *self, Uint32 dt)
{
    int xinc;
    int cursor_center;

    xinc = generic_ruler_get_pixel_increment_for(&self->ruler, SFV_GAUGE(self)->value);

    cursor_center = self->cursor->canvas->w/2;

    self->state.cursor_rect.x = (self->ruler.ruler_area.x + xinc)
                                + self->ruler_rect.x
                                - cursor_center;
}

static void fishbone_gauge_render(FishboneGauge *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        GENERIC_LAYER(&self->ruler),
        NULL,
        &self->ruler_rect);
    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        self->cursor,
        NULL,
        &self->state.cursor_rect);
}

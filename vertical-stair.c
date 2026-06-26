/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include <SDL2/SDL_image.h>
#include "SDL_pcf.h"

#include "SDL_surface.h"
#include "base-gauge.h"
#include "generic-layer.h"
#include "vertical-stair.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "misc.h"
#include "view.h"

static void vertical_stair_render(VerticalStair *self, Uint32 dt, RenderContext *ctx);
static void vertical_stair_update_state(VerticalStair *self, Uint32 dt);
static void *vertical_stair_dispose(VerticalStair *self);
static void vertical_stair_draw_background(VerticalStair *self, int width, int height,
                                           uint8_t base_unit_px, uint8_t small_step_width,
                                           uint8_t big_step_width, FontResource font);
static void vertical_stair_draw_cursor(VerticalStair *self);
static BaseGaugeOps vertical_stair_ops = {
   .render = (RenderFunc)vertical_stair_render,
   .update_state = (StateUpdateFunc)vertical_stair_update_state,
   .dispose = (DisposeFunc)vertical_stair_dispose
};


VerticalStair *vertical_stair_new(int width, int height, FontResource font)
{
    VerticalStair *self;

    self = calloc(1, sizeof(VerticalStair));
    if(self){
        if(!vertical_stair_init(self, width, height, font)){
            return base_gauge_free(BASE_GAUGE(self));
        }
    }
    return self;
}

VerticalStair *vertical_stair_init(VerticalStair *self, int width, int height, FontResource font)
{
    bool rv;


    self->font = resource_manager_get_static_font(font, &SDL_WHITE, 2, PCF_DIGITS, "+-");
    if(!self->font)
        return NULL;
    PCF_StaticFontRef(self->font);

    vertical_stair_draw_background(
        self, width, height,
        roundf(height*0.11f), roundf(width*0.24f),
        roundf(width*0.33f), font
    );
    vertical_stair_draw_cursor(self);
    /*TODO: Check if drawing worked*/

    generic_layer_build_texture(GENERIC_LAYER(&self->scale));
    generic_layer_build_texture(&self->cursor);

    /* TODO: Move me as first operation to ensure that Ops are always
     * set when we return NULL so that base_gauge_dispose (called by
     * base_gauge_free) can properly dispose any gauge-allocated resources*/
    base_gauge_init(BASE_GAUGE(self),
        &vertical_stair_ops,
        generic_layer_w(&self->cursor),
        generic_layer_h(GENERIC_LAYER(&self->scale))
    );

    return self;
}

static void *vertical_stair_dispose(VerticalStair *self)
{
    vertical_strip_dispose(&self->scale);
    generic_layer_dispose(&self->cursor);
    if(self->font)
        PCF_FreeStaticFont(self->font);
    return self;
}

bool vertical_stair_set_value(VerticalStair *self, float value, bool animated)
{
    return sfv_gauge_set_value(SFV_GAUGE(self), value, animated);
}


static void vertical_stair_update_state(VerticalStair *self, Uint32 dt)
{
    float y;
    char number[VS_VALUE_MAX_LEN];
    int ivalue;
    int len;
#if 0 /*Switch whether to align cursor text left/right*/
    uint8_t halign = HALIGN_RIGHT;
#else
    uint8_t halign = HALIGN_LEFT;
#endif

//    printf("%s %p value: %f\n", __FUNCTION__, self, SFV_GAUGE(self)->value);
    ivalue = round(SFV_GAUGE(self)->value);
    len = snprintf(number, VS_VALUE_MAX_LEN, "% -d", ivalue);
    vertical_strip_clip_value(&self->scale, &SFV_GAUGE(self)->value);

    y = vertical_strip_resolve_value(&self->scale, SFV_GAUGE(self)->value, true);
    y = round(round(y) - generic_layer_h(&self->cursor)/2.0);
    self->state.cloc = (SDL_Rect){
        1, y,
        generic_layer_w(&self->cursor),
        generic_layer_h(&self->cursor)
    };

    /*This non-tight size request will include left and right "margins"*/
    PCF_StaticFontGetSizeRequestRect(self->font, number, true, &self->state.tloc);
    SDLExt_RectAlign(&self->state.tloc,
        &self->state.cloc,
        halign | VALIGN_MIDDLE
    );
    if(halign == HALIGN_LEFT){
        /* Pretend a sign (+/-) is always present to avoid the text
         * "jumping" when the sign appears/disappears
         */
        self->state.tloc.x += self->font->metrics.characterWidth;
    }else{
        /*correct for the border width that is 1px*/
        self->state.tloc.x -= 1;
    }

    SDL_Rect glyph, cursor;
    cursor = self->state.tloc;
    self->state.nchars = PCF_StaticFontPreWriteString(self->font,
        len, number,
        true, &cursor,
        VS_VALUE_MAX_LEN-1, self->state.chars
    );
}

static void vertical_stair_render(VerticalStair *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        GENERIC_LAYER(&self->scale), NULL,
        &(SDL_Rect){
            0, 0,
            generic_layer_w(GENERIC_LAYER(&self->scale)),
            generic_layer_h(GENERIC_LAYER(&self->scale))
        }
    );

    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        &self->cursor, NULL, &self->state.cloc
    );

    for(int i = 0; i < self->state.nchars; i++){
        base_gauge_draw_static_font_patch(BASE_GAUGE(self),
            ctx,
            self->font,
            &self->state.chars[i]
        );
    }
}


static void vertical_stair_draw_background(VerticalStair *self, int width, int height,
                                           uint8_t base_unit_px, uint8_t small_step_width,
                                           uint8_t big_step_width, FontResource font)
{
    int line1_y_start = 0;
    int line2_y_start = 0;

    // assert height is odd ?


    bool rv = generic_layer_init(GENERIC_LAYER(&self->scale), width, height);
    if(!rv)
        return;

    int mid_y = height/2;
    line1_y_start = mid_y;
    line2_y_start = mid_y;

    int aperture = 0.16 * height;

    int line1_y_end = mid_y - aperture/2;
    int line1_x_end = (width-1);
    int line1_x_start = 0;

    int line2_y_end = mid_y + aperture/2;
    int line2_x_end = (width-1);
    int line2_x_start = 0;
    printf("%s: line1: %d,%d -> %d,%d\n", __FUNCTION__, 0, line1_y_start, line1_x_end, line1_y_end);
    printf("%s: line2: %d,%d -> %d,%d\n", __FUNCTION__, 0, line2_y_start, line2_x_end, line2_y_end);

    int dy = abs(line1_y_end - line1_y_start);

    SDL_Surface *canvas = GENERIC_LAYER(&self->scale)->canvas;
    view_draw_line(canvas, line1_x_start, line1_y_start, line1_x_end, line1_y_end, &(SDL_WHITE));
    view_draw_line(canvas, line2_x_start, line2_y_start, line2_x_end, line2_y_end, &(SDL_WHITE));

    self->scale.ppv = base_unit_px/500.0;
    self->scale.start = (500.0/base_unit_px) * (mid_y-(height-1));
    self->scale.end = (500.0/base_unit_px) * (mid_y - 0);


    SDL_LockSurface(canvas);
    Uint32 *pixels = canvas->pixels;
    Uint32 white = SDL_UWHITE(canvas);
    for(int x = 0; x < width; x++){
        pixels[0 * canvas->w + x] = white;
    }
    for(int x = 0; x < width; x++){
        pixels[(height-1) * canvas->w + x] = white;
    }

    for(int y = 0; y < height; y++){
        pixels[y * canvas->w + 0] = white;
        if(y > line1_y_end && y < line2_y_end)
            continue;

        pixels[y * canvas->w + (width-1)] = white;
    }

    int tick_drawn = 1; /* The midpoint is considered to be an undrawn tick (0)*/
    for(int y = mid_y-1; y >= 0; y--){ /*+1 skips the mid point tick*/
        if((y-mid_y) % base_unit_px == 0){ /* tick mark*/
            /* big tick mark, every two small ticks (500/1000) */
            uint8_t tick_width = (tick_drawn % 2 == 0) ? big_step_width : small_step_width;
            for(int x = 1; x <= tick_width; x++){
                pixels[y * canvas->w + x] = white;
            }
            tick_drawn++;
        }
    }

    tick_drawn = 1; /* The midpoint is considered to be an undrawn tick (0)*/
    for(int y = mid_y+1; y < height; y++){ /*+1 skips the mid point tick*/
        if((y-mid_y) % base_unit_px == 0){ /* tick mark*/
            /* big tick mark, every two small ticks (500/1000) */
            uint8_t tick_width = (tick_drawn % 2 == 0) ? big_step_width : small_step_width;
            for(int x = 1; x <= tick_width; x++){
                pixels[y * canvas->w + x] = white;
            }
            tick_drawn++;
        }
    }

    SDL_UnlockSurface(canvas);
    int offset = 5;
    PCF_Font *font_handle = resource_manager_get_font(font);
    for(int i = -2000; i <= 2000; i += 500){
        if(i == 0) continue; /* skip zero */
        if(i % 1000) continue; /* only draw big ticks*/

        int y = vertical_strip_resolve_value(&self->scale, i, true);
        int val = abs(i / 1000);
        PCF_FontWriteNumberAt(font_handle,
            &val, TypeInt, 0,
            white, false, canvas,
            big_step_width + offset, y, RightToCol | CenterOnRow);
    }
}

static void vertical_stair_draw_cursor(VerticalStair *self)
{
    Uint32 c_h, c_w;

    assert(self->font != NULL);

    PCF_StaticFontGetSizeRequest(self->font, "9999", false, &c_w, &c_h);
    c_w *= 1.2;

    int height = GENERIC_LAYER(&self->scale)->canvas->h;

    int mid_y = height/2;
    int aperture = 0.16 * height;
    aperture = MIN(aperture, c_h);
//    printf("%s: height: %d mid_y: %d aperture: %d\n", __FUNCTION__, height, mid_y, aperture);

    int aperture_y_first = mid_y - aperture/2;
    int aperture_y_last = mid_y + aperture/2;
//    printf("%s: aperture_y_first: %d aperture_y_last: %d\n", __FUNCTION__, aperture_y_first, aperture_y_last);


    int *frontiers = calloc(sizeof(int), aperture+1); /*+1 to account for the first row*/
    if(!frontiers){
        printf("%s: Failed to allocate frontiers\n", __FUNCTION__);
        return;
    }
    SDL_Surface *canvas = GENERIC_LAYER(&self->scale)->canvas;
    Uint32 white = SDL_UWHITE(canvas);
    SDL_LockSurface(canvas);
    Uint32 *pixels = canvas->pixels;
    for(int y = aperture_y_first; y <= aperture_y_last; y++){
        for(int x = 0; x < canvas->w-1; x++){
            Uint32 pixel =  pixels[(y*canvas->w) + x];
            if(pixel == white){
//                printf("Setting frontier[%d]: %d\n", y-aperture_y_first, x);
                frontiers[y - aperture_y_first] = x;
            }
        }
    }

    int ascent,descent;
    int local_aperture;
    for(int x = 1; x < canvas->w-1; x++){
        //for(ascent = mid_y; ascent >= aperture_y_first && pixels[(ascent*canvas->w) + x] != white; ascent--);
        for(ascent = mid_y; ascent >= aperture_y_first; ascent--){
            Uint32 pixel =  pixels[(ascent*canvas->w) + x];
            if(pixel == white){
                break;
            }
        }
        for(descent = mid_y; descent <= aperture_y_last; descent++){
            Uint32 pixel =  pixels[(descent*canvas->w) + x];
            if(pixel == white){
                break;
            }
        }
        local_aperture = abs(ascent-mid_y) + (descent-mid_y);
        if(local_aperture >= c_h){
       //     printf("Local aperture: %d >= cursor height: %d\n", local_aperture, c_h);
        //    printf("Old cursor width: %d New cursor width: %d\n", c_w, c_w + x);
            c_w += x;
            break;
        }
    }


    SDL_UnlockSurface(canvas);

    //printf("%s: cursor width: %d cursor height: %d\n", __FUNCTION__, c_w, c_h);
    bool rv = generic_layer_init(&self->cursor, c_w, c_h);
    if(!rv)
        return;
    canvas = self->cursor.canvas;
    SDL_LockSurface(canvas);
    pixels = canvas->pixels;
    Uint32 black = SDL_UBLACK(canvas);
    for(int y = 0; y < c_h && y < aperture; y++){
        for(int x = frontiers[y]+1; x < c_w; x++){
            pixels[(y*canvas->w) + x] = black;
        }
    }

#if 1 /*Switch wheter to draw a border on the cursor*/
    /* border */
    white = SDL_UWHITE(canvas);
    Uint32 amask = canvas->format->Amask;
    Uint8 ashift = canvas->format->Ashift;
    Uint8 alpha;
    for(int y = 0; y < c_h && y < aperture; y++){
        for(int x = frontiers[y]+1; x < c_w; x++){
            Uint32 *pixel = pixels + (y*canvas->w) + x;
            Uint32 *abel = y <= c_h/2
                           ? pixels + ((y-1)*canvas->w) + x
                           : pixels + ((y+1)*canvas->w) + x;
            if(*pixel != black) continue;
            if(y == 0 || y == c_h-1 || x == c_w-1){
                *pixel = white;
            } else {
                alpha = (*abel & amask) >> ashift;
                if(alpha != 0x00) continue;

                *pixel = white;
            }
        }
    }
#endif

    SDL_UnlockSurface(canvas);
    free(frontiers);
}

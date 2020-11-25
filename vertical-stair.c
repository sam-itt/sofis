#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_image.h>

#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "animated-gauge.h"
#include "buffered-gauge.h"
#include "generic-layer.h"
#include "misc.h"
#include "resource-manager.h"
#include "SDL_pcf.h"
#include "vertical-stair.h"
#include "sdl-colors.h"


static void vertical_stair_render_value(VerticalStair *self, float value);
static AnimatedGaugeOps vertical_stair_ops = {
   .render_value = (ValueRenderFunc)vertical_stair_render_value
};

VerticalStair *vertical_stair_new(const char *bg_img, const char *cursor_img, PCF_StaticFont *font)
{
    VerticalStair *self;

    self = calloc(1, sizeof(VerticalStair));
    if(self){
        if(!vertical_stair_init(self, bg_img, cursor_img, font)){
            free(self);
            return NULL;
        }
    }
    return self;
}


VerticalStair *vertical_stair_init(VerticalStair *self, const char *bg_img, const char *cursor_img, PCF_StaticFont *font)
{
    bool rv;

    rv = generic_layer_init_from_file(GENERIC_LAYER(&self->scale), bg_img);
    if(!rv)
        return NULL;
    self->scale.ppv = 43.0/1000.0; /*0.0426*/
    self->scale.start = -2279;
    self->scale.end = 2279;

    rv = generic_layer_init_from_file(&self->cursor, cursor_img);
    self->font = font;
    if(!rv || !self->font)
        return NULL; //TODO: Will leak self->scale.ruler
    self->font->refcnt++;
    generic_layer_build_texture(GENERIC_LAYER(&self->scale));
    generic_layer_build_texture(&self->cursor);

    animated_gauge_init(ANIMATED_GAUGE(self),
        ANIMATED_GAUGE_OPS(&vertical_stair_ops),
        generic_layer_w(&self->cursor),
        generic_layer_h(GENERIC_LAYER(&self->scale))
    );
    BUFFERED_GAUGE(self)->max_ops = 8;

    return self;
}

void vertical_stair_dispose(VerticalStair *self)
{
    animated_gauge_dispose(ANIMATED_GAUGE(self));
    vertical_strip_dispose(&self->scale);
    generic_layer_dispose(&self->cursor);
    if(self->font)
        PCF_FreeStaticFont(self->font);
}

void vertical_stair_free(VerticalStair *self)
{
    vertical_stair_dispose(self);
    free(self);
}

static void vertical_stair_render_value(VerticalStair *self, float value)
{
    float y;
    int ivalue;
    char number[6]; //5 digits plus \0
    SDL_Rect cloc; /*Cursor location*/
    SDL_Rect dst;

    ivalue = round(value);
    snprintf(number, 6, "% -d", ivalue);
    vertical_strip_clip_value(&self->scale, &value);

    buffered_gauge_clear(BUFFERED_GAUGE(self));
    buffered_gauge_blit_layer(BUFFERED_GAUGE(self),
        GENERIC_LAYER(&self->scale),
        NULL,
        &(SDL_Rect){0, 0,
            generic_layer_w(GENERIC_LAYER(&self->scale)),
            generic_layer_h(GENERIC_LAYER(&self->scale))
        }
    );

    y = vertical_strip_resolve_value(&self->scale, value, true);
    y = round(round(y) - generic_layer_h(&self->cursor)/2.0);
    cloc = (SDL_Rect){
        1, y,
        generic_layer_w(&self->cursor),
        generic_layer_h(&self->cursor)
    };
    buffered_gauge_blit_layer(BUFFERED_GAUGE(self), &self->cursor, NULL, &cloc);

    PCF_StaticFontGetSizeRequestRect(self->font, number, &dst);
    SDLExt_RectAlign(&dst, &cloc, HALIGN_LEFT | VALIGN_MIDDLE);
    dst.x += self->font->metrics.characterWidth;
#if USE_SDL_GPU
    buffered_gauge_static_font_draw_text(BUFFERED_GAUGE(self),
        &dst, 0,
        number,
        self->font,
        0xFFFF00FF
    );
#else
    buffered_gauge_static_font_draw_text(BUFFERED_GAUGE(self),
        &dst, 0,
        number,
        self->font,
        SDL_UCKEY(BUFFERED_GAUGE(self)->view)
    );
#endif
}

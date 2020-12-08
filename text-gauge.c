#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "SDL_gpu.h"
#include "SDL_pcf.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "base-gauge.h"
#include "generic-layer.h"
#include "text-gauge.h"
#include "sdl-colors.h"
#include "misc.h"

static void text_gauge_update_state(TextGauge *self, Uint32 dt);
static void text_gauge_render(TextGauge *self, Uint32 dt, RenderContext *ctx);
static BaseGaugeOps text_gauge_ops = {
   .render = (RenderFunc)text_gauge_render,
   .update_state = (StateUpdateFunc)text_gauge_update_state
};


TextGauge *text_gauge_new(const char *value, bool outlined, int w, int h)
{
    TextGauge *self;

    self = calloc(1, sizeof(TextGauge));
    if(self){
        if(!text_gauge_init(self, value, outlined, w, h)){
            free(self);
            return NULL;
        }
    }
    return self;
}

TextGauge *text_gauge_init(TextGauge *self, const char *value, bool outlined, int w, int h)
{
    base_gauge_init(BASE_GAUGE(self),
        &text_gauge_ops,
        w, h
    );

    if(value)
        text_gauge_set_value(self, value);
    self->outlined = outlined;
    return self;
}

void text_gauge_free(TextGauge *self)
{
    base_gauge_dispose(BASE_GAUGE(self));

    if(self->value)
        free(self->value);
    if(self->font.is_static)
        PCF_FreeStaticFont(self->font.static_font);
    else
        PCF_CloseFont(self->font.font);
    if(self->state.chars)
        free(self->state.chars);
#if USE_SDL_GPU
    if(self->buffer)
        generic_layer_free(self->buffer);
#endif
    free(self);
}

static void text_gauge_dispose_font(TextGauge *self)
{
    if(!self->font.is_static && self->font.font){
        self->font.font->xfont.refcnt--;
        PCF_CloseFont(self->font.font);
    }else if(self->font.static_font){
        self->font.static_font->refcnt--;
        PCF_FreeStaticFont(self->font.static_font);
    }
}

void text_gauge_set_font(TextGauge *self, PCF_Font *font)
{
    text_gauge_dispose_font(self);

    self->font.font = font;
    self->font.font->xfont.refcnt++;
    self->font.is_static = false;
}

void text_gauge_set_static_font(TextGauge *self, PCF_StaticFont *font)
{
    text_gauge_dispose_font(self);

    font->refcnt++;
    self->font.static_font = font;
    self->font.is_static = true;
}

void text_gauge_set_color(TextGauge *self, SDL_Color color, Uint8 which)
{
    if(which == TEXT_COLOR)
        self->text_color = color;
    else
        self->bg_color = color;
}

bool text_gauge_set_value(TextGauge *self, const char *value)
{
    int newlen;
    /*TODO: This is going to be quite mem-intesive, use a pool or something
     * and resort to allocation only when needing large pools*/
    newlen = strlen(value);
    if(newlen > self->asize){
        char *tmp;
        tmp = realloc(self->value, (newlen+1) * sizeof(char));
        if(!tmp) return false;
        self->value = tmp;
        self->asize = newlen;
    }

    if(newlen > self->state.achars){
        void *tmp;
        tmp = realloc(self->state.chars, newlen * sizeof(TextGaugeFontPatch));
        if(!tmp) return false;
        self->state.chars = tmp;

        self->state.achars = newlen;
    }

    strcpy(self->value, value);
    self->len = newlen;
    self->value[self->len] = '\0';

    BASE_GAUGE(self)->dirty = true;
    return true;
}


static inline void text_gauge_static_font_update_state(TextGauge *self, Uint32 dt)
{
    SDL_Rect farea;
    SDL_Rect glyph, cursor;
    PCF_StaticFont *sfont;

    if(!self->state.chars)
        return;

    sfont = self->font.static_font;

    farea = (SDL_Rect){
        .x = 0,
        .y = 0,
        .w = base_gauge_w(BASE_GAUGE(self)),
        .h = base_gauge_h(BASE_GAUGE(self))
    };

    PCF_StaticFontGetSizeRequestRect(sfont, self->value, &cursor);
    SDLExt_RectAlign(&cursor, &farea, self->alignment);
    /*avoid stretching when using SDL_Renderer*/
    cursor.w = sfont->metrics.characterWidth;

    self->state.nchars = 0;
    for(int i = 0; i < self->len; i++){
        if( PCF_StaticFontGetCharRect(sfont, self->value[i], &glyph) != 0){ /*0 means white space*/
            /*h and w are implied as the values found in sfont->metrics*/
            self->state.chars[self->state.nchars].src = (SDL_Point){glyph.x, glyph.y};
            self->state.chars[self->state.nchars].dst = (SDL_Point){cursor.x, cursor.y};
            self->state.nchars++;
        }
        cursor.x += sfont->metrics.characterWidth;
    }
}


static inline void text_gauge_regular_font_update_state(TextGauge *self, Uint32 dt)
{
    if(!self->buffer){
        self->buffer = generic_layer_new(base_gauge_w(BASE_GAUGE(self)), base_gauge_h(BASE_GAUGE(self)));
    }

    view_font_draw_text(self->buffer->canvas, NULL,
        self->alignment, self->value,
        self->font.font,
        SDL_MapRGBA(self->buffer->canvas->format,
            self->text_color.r, self->text_color.g,
            self->text_color.b, self->text_color.a
        ),
        SDL_MapRGBA(self->buffer->canvas->format,
            self->bg_color.r, self->bg_color.g,
            self->bg_color.b, self->bg_color.a
        )
    );
    generic_layer_update_texture(self->buffer);
    printf("redrawn backbuffer\n");
}

static void text_gauge_update_state(TextGauge *self, Uint32 dt)
{
    if(!self->font.font) return; /*Wait until either font has been set*/

    if(self->font.is_static){
        text_gauge_static_font_update_state(self, dt);
    }else{
        text_gauge_regular_font_update_state(self, dt);
    }
}

static inline void text_gauge_static_font_render(TextGauge *self, Uint32 dt,
                                                 RenderContext *ctx)
{

    base_gauge_fill(BASE_GAUGE(self), ctx, NULL, &self->bg_color, false);
    if(self->outlined)
        base_gauge_draw_outline(BASE_GAUGE(self), ctx, &SDL_WHITE, NULL);

    for(int i = 0; i < self->state.nchars; i++){
        base_gauge_draw_static_font_glyph(BASE_GAUGE(self),
            ctx,
            self->font.static_font,
            &self->state.chars[i].src,
            &self->state.chars[i].dst
        );
    }
}

static inline void text_gauge_regular_font_render(TextGauge *self, Uint32 dt,
                                                 RenderContext *ctx)
{

    base_gauge_fill(BASE_GAUGE(self), ctx, NULL, &self->bg_color, false);
    base_gauge_blit_layer(BASE_GAUGE(self), ctx, self->buffer, NULL, NULL);

    if(self->outlined)
        base_gauge_draw_outline(BASE_GAUGE(self), ctx, &SDL_WHITE, NULL);
}


static void text_gauge_render(TextGauge *self, Uint32 dt, RenderContext *ctx)
{
    if(!self->font.font) return; /*Wait until either font has been set*/

    if(self->font.is_static){
        text_gauge_static_font_render(self, dt, ctx);
    }else{
        text_gauge_regular_font_render(self, dt, ctx);
    }
}

#if 0
static void text_gauge_render(TextGauge *self, Uint32 dt)
{
    if(!self->font.font) return; /*Wait until either font has been set*/

    if(self->value){
        if(self->font.is_static)
            buffered_gauge_static_font_draw_text(BUFFERED_GAUGE(self), NULL, self->alignment, self->value, self->font.static_font, self->bg_color);
        else
            buffered_gauge_font_draw_text(BUFFERED_GAUGE(self), NULL, self->alignment, self->value, self->font.font, self->text_color, self->bg_color);
    }else{
        buffered_gauge_fill(BUFFERED_GAUGE(self), NULL, &self->bg_color, true);
    }

    if(self->outlined)
        buffered_gauge_draw_outline(BUFFERED_GAUGE(self), &SDL_WHITE, NULL);
}
#endif

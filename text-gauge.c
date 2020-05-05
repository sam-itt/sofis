#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "SDL_pixels.h"
#include "buffered-gauge.h"
#include "sdl-colors.h"
#include "sdl-pcf/SDL_pcf.h"
#include "text-gauge.h"

static void text_gauge_render(TextGauge *self, Uint32 dt);

static BufferedGaugeOps text_gauge_ops = {
    .render = (BufferRenderFunc)text_gauge_render
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
    buffered_gauge_init(BUFFERED_GAUGE(self),
        BUFFERED_GAUGE_OPS(&text_gauge_ops),
        w, h
    );

    if(value)
        text_gauge_set_value(self, value);
    self->outlined = outlined;
    return self;
}

void text_gauge_free(TextGauge *self)
{
    buffered_gauge_dispose(BUFFERED_GAUGE(self));

    if(self->value)
        free(self->value);
    if(self->font.is_static)
        PCF_FreeStaticFont(self->font.static_font);
    else
        PCF_CloseFont(self->font.font);
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

void text_gauge_build_static_font(TextGauge *self, PCF_Font *font, SDL_Color *color, int nsets, ...)
{
    va_list ap;
    char *tmp;
    size_t tlen;
    PCF_StaticFont *sfont;

    tlen = 0;
    va_start(ap, nsets);
    for(int i = 0; i < nsets; i++){
        tmp = va_arg(ap, char*);
        tlen += strlen(tmp);
    }
    va_end(ap);

    text_gauge_dispose_font(self);
    va_start(ap, nsets);
    sfont = PCF_FontCreateStaticFontVA(font, color, nsets, tlen, ap);
    va_end(ap);
    if(!sfont) return; //TODO: Handle failure ?

    text_gauge_set_static_font(self, sfont);
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
    Uint32 icol;

    icol = SDL_MapRGBA(buffered_gauge_get_view(BUFFERED_GAUGE(self))->format, color.r, color.g, color.b, color.a);
    if(which == TEXT_COLOR)
        self->text_color = icol;
    else
        self->bg_color = icol;
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
    }
    strcpy(self->value, value);
    self->len = newlen;
    self->value[self->len] = '\0';

    BUFFERED_GAUGE(self)->damaged = true;
    return true;
}

static void text_gauge_render(TextGauge *self, Uint32 dt)
{
    if(!self->font.font) return; /*Wait until either font has been set*/

    if(self->value){
        if(self->font.is_static)
            buffered_gauge_static_font_draw_text(BUFFERED_GAUGE(self), NULL, self->alignment, self->value, self->font.static_font, self->bg_color);
        else
            buffered_gauge_font_draw_text(BUFFERED_GAUGE(self), NULL, self->alignment, self->value, self->font.font, self->text_color, self->bg_color);
    }else{
        buffered_gauge_clear_color(BUFFERED_GAUGE(self), self->bg_color);
    }

    if(self->outlined)
        buffered_gauge_draw_outline(BUFFERED_GAUGE(self), &SDL_WHITE, NULL);
}

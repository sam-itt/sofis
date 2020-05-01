#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_image.h>

#include "SDL_surface.h"
#include "animated-gauge.h"
#include "buffered-gauge.h"
#include "resource-manager.h"
#include "sdl-pcf/SDL_pcf.h"
#include "vertical-stair.h"
#include "sdl-colors.h"


static void vertical_stair_render_value(VerticalStair *self, float value);
static AnimatedGaugeOps vertical_stair_ops = {
   .render_value = (ValueRenderFunc)vertical_stair_render_value
};



VerticalStairCursor *vertical_stair_cursor_init(VerticalStairCursor *self, const char *filename, PCF_Font *font)
{
    self->bg = IMG_Load(filename);
    self->value = NAN;
    self->font = font;
    self->font->xfont.refcnt++;
    if(!self->bg || !self->font)
        return NULL;
    return self;
}

void vertical_stair_cursor_dispose(VerticalStairCursor *self)
{
    if(self->bg)
        SDL_FreeSurface(self->bg);
    if(self->font)
        PCF_CloseFont(self->font);
}

void vertical_stair_cursor_set_value(VerticalStairCursor *self, float value)
{
    SDL_Surface *text;
    SDL_Rect dst;
    char number[6]; //5 digits plus \0
    int ivalue;

    if(self->value != value){
        Uint32 black = SDL_UBLACK(self->bg);
        SDL_FillRect(self->bg, &(SDL_Rect){5,5,8,7}, black);
        SDL_FillRect(self->bg, &(SDL_Rect){13,0,self->bg->w-13,self->bg->h}, black);

        ivalue = round(value);
        snprintf(number, 6, "% -d", ivalue);

        dst = (SDL_Rect){7, 1,0,0};
        PCF_FontWrite(self->font, number, SDL_UWHITE(self->bg), self->bg, &dst); /*TODO: StaticFont ?*/

        self->value = value;
    }

}


VerticalStair *vertical_stair_new(const char *bg_img, const char *cursor_img, PCF_Font *font)
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


VerticalStair *vertical_stair_init(VerticalStair *self, const char *bg_img, const char *cursor_img, PCF_Font *font)
{

    self->scale.ruler = IMG_Load(bg_img);
    self->scale.ppv = 43.0/1000.0; /*0.0426*/
    self->scale.start = -2279;
    self->scale.end = 2279;

    vertical_stair_cursor_init(&self->cursor, cursor_img, font);
    animated_gauge_init(ANIMATED_GAUGE(self), ANIMATED_GAUGE_OPS(&vertical_stair_ops), self->cursor.bg->w, self->scale.ruler->h);

    return self;
}

void vertical_stair_free(VerticalStair *self)
{
    animated_gauge_dispose(ANIMATED_GAUGE(self));
    vertical_strip_dispose(&self->scale);
    vertical_stair_cursor_dispose(&self->cursor);
    free(self);
}

static void vertical_stair_render_value(VerticalStair *self, float value)
{
    float y;

    vertical_stair_cursor_set_value(&self->cursor, value);
    vertical_strip_clip_value(&self->scale, &value);

    buffered_gauge_clear(BUFFERED_GAUGE(self), NULL);
    buffered_gauge_blit(BUFFERED_GAUGE(self), self->scale.ruler, NULL, NULL);

    y = vertical_strip_resolve_value(&self->scale, value, true);
    y = round(round(y) - self->cursor.bg->h/2.0);

    buffered_gauge_blit(BUFFERED_GAUGE(self), self->cursor.bg, NULL, &(SDL_Rect){1, y,0,0});
}

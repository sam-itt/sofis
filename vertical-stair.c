#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "vertical-stair.h"
#include "sdl-colors.h"


static void vertical_stair_render_value(VerticalStair *self, float value);

VerticalStairCursor *vertical_stair_cursor_init(VerticalStairCursor *self, const char *filename, int font_size)
{
    self->bg = IMG_Load(filename);
    self->value = NAN;
    self->font  = TTF_OpenFont("TerminusTTF-4.47.0.ttf", font_size);
    if(!self->bg || !self->font)
        return NULL;
    return self;
}

void vertical_stair_cursor_dispose(VerticalStairCursor *self)
{
    if(self->bg)
        SDL_FreeSurface(self->bg);
    if(self->font)
        TTF_CloseFont(self->font);
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
        text = TTF_RenderText_Solid(self->font, number, SDL_WHITE);

        dst = (SDL_Rect){7,-1,0,0};
        SDL_BlitSurface(text, NULL, self->bg, &dst);
        SDL_FreeSurface(text);

        self->value = value;
    }

}


VerticalStair *vertical_stair_new(const char *bg_img, const char *cursor_img, int cfont_size)
{
    VerticalStair *self;

    self = calloc(1, sizeof(VerticalStair));
    if(self){
        if(!vertical_stair_init(self, bg_img, cursor_img, cfont_size)){
            free(self);
            return NULL;
        }
    }
    return self;
}


VerticalStair *vertical_stair_init(VerticalStair *self, const char *bg_img, const char *cursor_img, int cfont_size)
{
    ANIMATED_GAUGE(self)->renderer = (ValueRenderFunc)vertical_stair_render_value;
    ANIMATED_GAUGE(self)->damaged = true;

    self->scale.ruler = IMG_Load(bg_img);
    self->scale.ppv = 43.0/1000.0; /*0.0426*/
    self->scale.fei = 11;
    self->scale.vstep = 1000;
    self->scale.start = -2279;
    self->scale.end = 2279;

    vertical_stair_cursor_init(&self->cursor, cursor_img, cfont_size);
//    ANIMATED_GAUGE(self)->view = SDL_CreateRGBSurface(0, self->scale.ruler->w,self->scale.ruler->h, 32,0,0,0,0);
    ANIMATED_GAUGE(self)->view = SDL_CreateRGBSurface(0, self->cursor.bg->w, self->scale.ruler->h, 32,0,0,0,0);
    SDL_SetColorKey(ANIMATED_GAUGE(self)->view, SDL_TRUE, SDL_UCKEY(ANIMATED_GAUGE(self)->view));

    return self;
}

void vertical_stair_free(VerticalStair *self)
{
    animated_gauge_dispose(ANIMATED_GAUGE(self));
    vertical_strip_dispose(&self->scale);
    vertical_stair_cursor_dispose(&self->cursor);
    free(self);
}

/**
 * TODO: Find a way to integrate this in vertical_strip_resolve_value
 * without breaking everything
 */
float vertical_stair_resolve_value(VerticalStair *self, float value)
{
    float rv;

    /* To map [A, B] --> [a, b]
     *
     * use this formula : (val - A)*(b-a)/(B-A) + a
     *
     */

/*    first interval is start,end, second interval is 0,190*/
    rv = (value - self->scale.start)*((self->scale.ruler->h-1) - 0)/(self->scale.end - self->scale.start) + 0;
    rv = (self->scale.ruler->h-1) - rv;
    return rv;
}


static void vertical_stair_render_value(VerticalStair *self, float value)
{
    float y;

    vertical_stair_cursor_set_value(&self->cursor, value);
    vertical_strip_clip_value(&self->scale, &value);
    SDL_FillRect(ANIMATED_GAUGE(self)->view, NULL, SDL_UCKEY(ANIMATED_GAUGE(self)->view));
    SDL_BlitSurface(self->scale.ruler, NULL, ANIMATED_GAUGE(self)->view, NULL);

    //y = vertical_strip_resolve_value(&self->scale, value, true);
    y = vertical_stair_resolve_value(self, value);
    y = round(round(y) - self->cursor.bg->h/2.0);

    SDL_BlitSurface(self->cursor.bg, NULL, ANIMATED_GAUGE(self)->view, &(SDL_Rect){1, y,0,0});

}

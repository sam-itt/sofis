#ifndef VIEW_H
#define VIEW_H

#include <SDL2/SDL.h>

#include "sdl-pcf/SDL_pcf.h"

typedef struct{
    union{
        PCF_Font *font;
        PCF_StaticFont *static_font;
    };
    bool is_static;
}PCFWrapFont;

void view_clear(SDL_Surface *self, SDL_Rect *area);
void view_draw_outline(SDL_Surface *self, SDL_Color *rgba, SDL_Rect *area);

void view_font_draw_text(SDL_Surface *destination, SDL_Rect *location, uint8_t alignment, const char *string, PCF_Font *font, Uint32 text_color, Uint32 bg_color);

void view_draw_rubis(SDL_Surface *surface, int y, SDL_Color *color, int pskip, SDL_Rect *clip);
#endif /* VIEW_H */

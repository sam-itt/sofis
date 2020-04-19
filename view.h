#ifndef VIEW_H
#define VIEW_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

void view_clear(SDL_Surface *self);
void view_draw_outline(SDL_Surface *self, SDL_Color *rgba, SDL_Rect *area);

void view_draw_text(SDL_Surface *destination, SDL_Rect *location, const char *string, TTF_Font *font, SDL_Color *text_color, SDL_Color *bg_color);

#endif /* VIEW_H */

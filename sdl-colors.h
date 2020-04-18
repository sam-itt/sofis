#ifndef SDL_COLORS_H
#define SDL_COLORS_H

#define SDL_WHITE (SDL_Color){255, 255, 255, SDL_ALPHA_OPAQUE}
#define SDL_RED (SDL_Color){255, 0, 0, SDL_ALPHA_OPAQUE}
#define SDL_BLACK (SDL_Color){0, 0, 0, SDL_ALPHA_OPAQUE}


#define SDL_UBLACK(surface) (SDL_MapRGB((surface)->format, 0, 0,0))
#define SDL_URED(surface) (SDL_MapRGB((surface)->format, 255, 0,0))
#define SDL_UGREEN(surface) (SDL_MapRGB((surface)->format, 0, 255,0))
#define SDL_UWHITE(surface) (SDL_MapRGB((surface)->format, 255, 255,255))
#define SDL_UYELLOW(surface) (SDL_MapRGB((surface)->format, 255, 255,0))
#define SDL_UGREY(surface) (SDL_MapRGB((surface)->format, 205, 205,205))
#define SDL_UFBLUE(surface) (SDL_MapRGB((surface)->format, 0x11, 0x56, 0xFF)) //Fancy blue

#define SDL_UCKEY(surface) (SDL_MapRGB((surface)->format, 255, 0,255))


#endif /* SDL_COLORS_H */

#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "SDL_pcf.h"


typedef enum{
    TERMINUS_12,
    TERMINUS_14,
    TERMINUS_16,
    TERMINUS_18,
    TERMINUS_24,
    TERMINUS_32,

    FONT_MAX
}FontResource;

typedef struct{
    PCF_StaticFont *font;
    SDL_Color color;
    FontResource creator;
}StaticFontResource;

typedef struct{
    PCF_Font *fonts[FONT_MAX];

    StaticFontResource *sfonts;
    size_t n_allocated;
    size_t n_sfonts;
}ResourceManager;

PCF_Font *resource_manager_get_font(FontResource font);
PCF_StaticFont *resource_manager_get_static_font(FontResource font, SDL_Color *color, int nsets, ...);

void resource_manager_shutdown(void);
#endif /* RESOURCE_MANAGER_H */

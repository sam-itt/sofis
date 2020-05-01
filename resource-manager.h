#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "sdl-pcf/SDL_pcf.h"


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
    PCF_Font *fonts[FONT_MAX];

}ResourceManager;

PCF_Font *resource_manager_get_font(FontResource font);
void resource_manager_shutdown(void);
#endif /* RESOURCE_MANAGER_H */

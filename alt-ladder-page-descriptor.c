#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_image.h>

#include "alt-ladder-page-descriptor.h"
#include "animated-gauge.h"
#include "resource-manager.h"
#include "sdl-colors.h"

#define PAGE_SIZE 700 /*number of values per page*/


AltLadderPageDescriptor *alt_ladder_page_descriptor_new(void)
{
    AltLadderPageDescriptor *self;
    FBPageDescriptor *tmp;

    self = calloc(1, sizeof(AltLadderPageDescriptor));
    if(self){
        tmp = fb_page_descriptor_init((FBPageDescriptor *)self, "alt-ladder.png",BOTTUM_UP, PAGE_SIZE, 100, 20);
        if(!tmp){
            free(self);
            return NULL;
        }
        self->super.super.init_page = alt_ladder_page_init;
        self->super.super.fei = 227;
    }
    return self;
}

LadderPage *alt_ladder_page_init(LadderPage *self)
{
    fb_ladder_page_init(self);
    ladder_page_etch_markings(self, resource_manager_get_font(TERMINUS_16));
#if USE_SDL_RENDERER
    VERTICAL_STRIP(self)->rtex = SDL_CreateTextureFromSurface(g_renderer, VERTICAL_STRIP(self)->ruler);
#endif
    return self;
}


#include <stdio.h>
#include <stdlib.h>

#include "resource-manager.h"
#include "sdl-pcf/SDL_pcf.h"

static ResourceManager *_instance = NULL;

static ResourceManager *resource_manager_new(void)
{
    ResourceManager *self;

    self = calloc(1, sizeof(ResourceManager));
    if(self){
        /*init as needed*/
    }
    return self;
}

static inline ResourceManager *resource_manager_get_instance(void)
{
    if(!_instance)
        _instance = resource_manager_new();
    return _instance;
}

void resource_manager_shutdown(void)
{
    if(_instance){
        for(int i = 0; i < FONT_MAX; i++){
            if(_instance->fonts[i]){
                if(_instance->fonts[i]->xfont.refcnt > 1){
                    printf(
                        "ResourceManager: Font %d refcount was still %d at shutdown (1 expected), leaking %p\n",
                        i,
                        _instance->fonts[i]->xfont.refcnt,
                        _instance->fonts[i]
                    );
                }
                _instance->fonts[i]->xfont.refcnt--;
                PCF_CloseFont(_instance->fonts[i]);
            }
        }
    }
}

static const char *resource_manager_get_font_filename(FontResource font)
{
    switch (font) {
        case TERMINUS_12:
            return "fonts/ter-x12n.pcf.gz";
        case TERMINUS_14:
            return "fonts/ter-x14n.pcf.gz";
        case TERMINUS_16:
            return "fonts/ter-x16n.pcf.gz";
        case TERMINUS_18:
            return "fonts/ter-x18n.pcf.gz";
        case TERMINUS_24:
            return "fonts/ter-x24n.pcf.gz";
        case TERMINUS_32:
            return "fonts/ter-x32n.pcf.gz";
        case FONT_MAX:
        default:
            return NULL;
    }
}

PCF_Font *resource_manager_get_font(FontResource font)
{
    ResourceManager *self;

    self = resource_manager_get_instance();
    if(!self->fonts[font]){
        self->fonts[font] = PCF_OpenFont(resource_manager_get_font_filename(font));
        if(self->fonts[font])
            self->fonts[font]->xfont.refcnt++;
    }
    return self->fonts[font];
}

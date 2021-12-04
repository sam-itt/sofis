#ifndef BUTTON_H
#define BUTTON_H


#include "SDL_pcf.h"

#include "base-widget.h"
#include "resource-manager.h"

typedef struct{
    PCF_StaticFontPatch *patches;
    int npatches;
}ButtonState;


typedef struct {
    BaseWidget super;

    FontResource font_id;
    PCF_StaticFont *sfont;

    uint8_t alignment;

    char *caption;
    size_t clen; /*strlen(caption)*/

    ButtonState state;

    EventListener validated;
}Button;

Button *button_new(const char *caption, FontResource font_id, int w, int h);
Button *button_init(Button *self, const char *caption, FontResource font_id, int w, int h);
#endif /* BUTTON_H */

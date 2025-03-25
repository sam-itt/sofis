#ifndef BUTTON_FLAT_H

#include "text-gauge.h"
#include "base-widget.h"
#include "resource-manager.h"


typedef struct {
    BaseWidget super;

    uintf8_t border_width;

    FontResource font_id;
    TextGauge *text;

    EventListener validated;
}ButtonFlat;

#define BUTTON_FLAT(self) ((ButtonFlat *)(self))

ButtonFlat *button_flat_new(const char *caption, FontResource font_id, SDL_Color text_color, SDL_Color bg_color, int w, int h);
ButtonFlat *button_flat_init(ButtonFlat *self, const char *caption, FontResource font_id, SDL_Color text_color, SDL_Color bg_color, int w, int h, uintf8_t border_width);

bool button_flat_set_caption(ButtonFlat *self, const char *caption);
bool button_flat_set_color(ButtonFlat *self, SDL_Color color, Uint8 which);
#define BUTTON_FLAT_H
#endif /* BUTTON_FLAT_H */

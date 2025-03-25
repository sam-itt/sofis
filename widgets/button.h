#ifndef BUTTON_H
#define BUTTON_H

#include "text-gauge.h"
#include "base-widget.h"
#include "resource-manager.h"

typedef struct {
    BaseWidget super;

    FontResource font_id;
    TextGauge *text;

    EventListener validated;
}Button;

#define BUTTON(self) ((Button *)(self))

Button *button_new(const char *caption, FontResource font_id, SDL_Color text_color, SDL_Color bg_color, int w, int h);
Button *button_init(Button *self, const char *caption, FontResource font_id, SDL_Color text_color, SDL_Color bg_color, int w, int h);

bool button_set_caption(Button *self, const char *caption);
bool button_set_color(Button *self, SDL_Color color, Uint8 which);
#endif /* BUTTON_H */

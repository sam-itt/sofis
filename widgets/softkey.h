#ifndef SOFTKEY_H

#include "button.h"

typedef enum{SOFTKEY_STATE_RELEASED, SOFTKEY_STATE_PRESSED, SOFTKEY_STATE_DISABLED } SoftkeyState;

typedef struct {
    Button super;

    SoftkeyState state;
}Softkey;


Softkey *softkey_new(const char *caption, FontResource font_id, int w, int h);
Softkey *softkey_init(Softkey *self, const char *caption, FontResource font_id, int w, int h);

void softkey_set_state(Softkey *self, SoftkeyState state);
#define SOFTKEY_H
#endif /* SOFTKEY_H */

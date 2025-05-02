#ifndef SOFTKEY_MODEL_H
#define SOFTKEY_MODEL_H

#include "softkey.h"
#include <stdint.h>

#define SOFTKEY_CAPTION_MAX 9 /*8 chars plus NUL*/

typedef struct _SoftkeyModel SoftkeyModel;

typedef enum {SOFTKEY_TYPE_REGULAR, SOFTKEY_TYPE_SEGUE, SOFTKEY_TYPE_BACK} SoftkeyType;
typedef struct{
    SoftkeyState state;
    SoftkeyType type;
    union{
        void (*clickedCallback)(void);
        SoftkeyModel *next;
    }action;
    char caption[SOFTKEY_CAPTION_MAX];
}SoftkeyDetails;

#define SOFTKEY_DETAILS(self) ((SoftkeyDetails*)(self))

typedef void* (*DisposeFunc)(void *self); /*TODO: Move upwards*/
typedef SoftkeyDetails* (*GetButtonDetailsAtFunc)(SoftkeyModel *self, uint_fast8_t index);

typedef struct{
    DisposeFunc dispose;
    GetButtonDetailsAtFunc get_details_at;
}SoftkeyModelOps;

typedef struct _SoftkeyModel{
    SoftkeyModelOps *ops;
    SoftkeyModel *prev;
}SoftkeyModel;

#define SOFTKEY_MODEL(self) ((SoftkeyModel *)(self))
#define SOFTKEY_MODEL_OPS(self) ((SoftkeyModelOps *)(self))


SoftkeyModel *softkey_model_init(SoftkeyModel *self, SoftkeyModelOps *ops);
SoftkeyModel *softkey_model_dispose(SoftkeyModel *self);

SoftkeyDetails *softkey_model_get_details_at(SoftkeyModel *self, uint_fast8_t index);
#endif /* SOFTKEY_MODEL_H */

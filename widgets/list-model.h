#ifndef LIST_MODEL_H
#define LIST_MODEL_H
#include <stdlib.h>

typedef void* (*DisposeFunc)(void *self);

typedef struct _ListBox ListBox;

typedef struct{
    DisposeFunc dispose;
}ListModelOps;

typedef struct{
    char *label;
    void *key;
}ListModelRow;

typedef struct{
    ListModelOps *ops;

    ListBox *listbox;
    /**
     *  This is the standard interface used by ListBox
     *  ListModel derivatives should fill those with
     *  appropriate data.
     * */
    ListModelRow *rows;
    size_t nrows;
    size_t arows;
    size_t *row_lenghts;
    size_t maxlen; /*strlen of the largest row*/
}ListModel;

#define LIST_MODEL(self) ((ListModel *)(self))
#define LIST_MODEL_OPS(self) ((ListModelOps *)(self))

ListModel *list_model_init(ListModel *self, ListModelOps *ops, int arows);
ListModel *list_model_dispose(ListModel *self);

static inline ListModel *list_model_free(ListModel *self)
{
    if(self->ops->dispose)
        self->ops->dispose(self);
    free(self);
    return NULL;
}

#endif /* LIST_MODEL_H */

#include <stdlib.h>

#include "list-model.h"

/**
 * @brief Allocate @param arows
 *
 */
ListModel *list_model_init(ListModel *self, ListModelOps *ops, int arows)
{
    self->ops = ops;

    self->rows = malloc(sizeof(ListModelRow)*arows);
    if(!self->rows)
        return NULL;

    self->row_lenghts = malloc(sizeof(size_t)*arows);
    if(!self->row_lenghts)
        return NULL;

    self->arows = arows;

    return self;
}

ListModel *list_model_dispose(ListModel *self)
{
    if(self->rows)
        free(self->rows);
    if(self->arows)
        free(self->row_lenghts);

    return self;
}

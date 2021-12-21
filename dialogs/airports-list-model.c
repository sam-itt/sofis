#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "misc.h"
#include "list-box.h"
#include "airports-list-model.h"
#include "dialogs/airport.h"

static AirportListModel *airport_list_model_dispose(AirportListModel *self);
static ListModelOps airport_list_model_ops = {
    .dispose = (DisposeFunc)airport_list_model_dispose
};

AirportListModel *airport_list_model_new()
{
    AirportListModel *self;

    self = calloc(1, sizeof(AirportListModel));
    if(self){
        if(!airport_list_model_init(self))
            return (AirportListModel*)list_model_free(LIST_MODEL(self));
    }
    return self;
}

AirportListModel *airport_list_model_init(AirportListModel *self)
{
    if(!list_model_init(LIST_MODEL(self), &airport_list_model_ops, nfrench_airports))
        return NULL;

    self->nfullnames = nfrench_airports;
    self->fullnames = malloc(sizeof(char *) * self->nfullnames);
    if(!self->fullnames)
        return NULL;
#if 0
    size_t binsize = 0;
    for(int i = 0; i < nfrench_airports; i++){
        binsize += strlen(french_airports[i].code);
        binsize += 3; /* strlen(" - ") */
        binsize += strlen(french_airports[i].name);
        binsize += 1; /*final \0 byte */
    }

    self->namestash = malloc(sizeof(char)*binsize);
    if(!self->namestash)
        return NULL;

    char *stashptr = self->namestash;
    for(int i = 0; i < nfrench_airports; i++){
        self->fullnames[i] = stashptr;
        strcat(stashptr, french_airports[i].code);
        strcat(stashptr, " - ");
        strcat(stashptr, french_airports[i].name);
        while(*stashptr != '\0') stashptr++;
        stashptr++;
    }
#else
     for(int i = 0; i < nfrench_airports; i++){
         asprintf(&self->fullnames[i],
            "%s - %s",
            french_airports[i].code,
            french_airports[i].name
        );
    }
#endif
    LIST_MODEL(self)->maxlen = 0;
    for(int i = 0; i < self->nfullnames; i++){
        LIST_MODEL(self)->rows[i].key = &french_airports[i];
        LIST_MODEL(self)->rows[i].label = self->fullnames[i];
        LIST_MODEL(self)->row_lenghts[i] = strlen(self->fullnames[i]);
        LIST_MODEL(self)->maxlen = MAX(
            LIST_MODEL(self)->maxlen,
            LIST_MODEL(self)->row_lenghts[i]
        );
    }
    LIST_MODEL(self)->nrows = self->nfullnames;

    return self;
}

static AirportListModel *airport_list_model_dispose(AirportListModel *self)
{
    list_model_dispose(LIST_MODEL(self));
    if(self->fullnames){
#if 1
        for(int i = 0; i < nfrench_airports; i++){
            if(self->fullnames[i])
                free(self->fullnames[i]);
        }
#endif
        free(self->fullnames);
    }
    if(self->namestash)
        free(self->namestash);
    return self;
}

void airport_list_model_filter(AirportListModel *self, const char *filter)
{
    ListModel *lself = LIST_MODEL(self);

    lself->nrows = 0;
    for(int i = 0; i < self->nfullnames; i++){
        if(strcasestr(self->fullnames[i], filter)){
            lself->rows[lself->nrows].key = &french_airports[i];
            lself->rows[lself->nrows].label = self->fullnames[i];
            lself->row_lenghts[lself->nrows] = strlen(self->fullnames[i]);
            lself->nrows++;
        }
    }
    list_box_model_changed(lself->listbox);
}


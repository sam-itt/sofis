#ifndef AIRPORTS_LIST_MODEL_H
#define AIRPORTS_LIST_MODEL_H
#include "list-model.h"
#include "airport.h"

typedef struct{
    ListModel super;

    char *namestash;

    char **fullnames;
    size_t nfullnames;
}AirportListModel;


AirportListModel *airport_list_model_new();
AirportListModel *airport_list_model_init(AirportListModel *self);

void airport_list_model_filter(AirportListModel *self, const char *filter);
#endif /* AIRPORTS_LIST_MODEL_H */

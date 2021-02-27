#ifndef FG_DATA_SOURCE_H
#define FG_DATA_SOURCE_H

#include "data-source.h"
#include "flightgear-connector.h"

typedef struct{
    DataSource super;

    FlightgearConnector *fglink;
}FGDataSource;

FGDataSource *fg_data_source_new(int port);
FGDataSource *fg_data_source_init(FGDataSource *self, int port);

#endif /* FG_DATA_SOURCE_H */

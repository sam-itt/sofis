#ifndef STRATUX_DATA_SOURCE_H
#define STRATUX_DATA_SOURCE_H
#include "data-source.h"
#include "http-buffer.h"

typedef struct{
    DataSource super;

    HttpBuffer *buf;
}StratuxDataSource;


StratuxDataSource *stratux_data_source_new();
StratuxDataSource *stratux_data_source_init(StratuxDataSource *self);
#endif /* STRATUX_DATA_SOURCE_H */

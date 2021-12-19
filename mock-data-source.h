#ifndef MOCK_DATA_SOURCE_H
#define MOCK_DATA_SOURCE_H
#include "data-source.h"

typedef struct{
    DataSource super;
}MockDataSource;

MockDataSource *mock_data_source_new(void);
MockDataSource *mock_data_source_init(MockDataSource *self);

#endif /* MOCK_DATA_SOURCE_H */

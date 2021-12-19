#include "data-source.h"

static DataSource *_datasource = NULL;

DataSource *data_source_get(void)
{
    return _datasource;
}

void data_source_set(DataSource *source)
{
    _datasource = source;
}

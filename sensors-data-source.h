#ifndef SENSORS_DATA_SOURCE_H
#define SENSORS_DATA_SOURCE_H

#include "data-source.h"
#include "sensors/jy61.h"

typedef struct{
    DataSource super;

    JY61 jy61_dev;
}SensorsDataSource;

SensorsDataSource *sensors_data_source_new(void);
SensorsDataSource *sensors_data_source_init(SensorsDataSource *self);

#endif /* SENSORS_DATA_SOURCE_H */

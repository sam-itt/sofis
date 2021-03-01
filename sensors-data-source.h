#ifndef SENSORS_DATA_SOURCE_H
#define SENSORS_DATA_SOURCE_H

#include "data-source.h"
#include "sensors/jy61.h"
#include "sensors/lsm303.h"
#include "sensors/gps-sensor.h"

typedef struct{
    DataSource super;

    JY61 jy61_dev;
    GpsSensor gps;
    Lsm303 mag;
}SensorsDataSource;

SensorsDataSource *sensors_data_source_new(void);
SensorsDataSource *sensors_data_source_init(SensorsDataSource *self);

#endif /* SENSORS_DATA_SOURCE_H */

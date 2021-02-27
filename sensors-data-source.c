#include <stdio.h>
#include <stdlib.h>

#include "sensors-data-source.h"

static bool sensors_data_source_frame(SensorsDataSource *self, uint32_t dt);
static SensorsDataSource *sensors_data_source_dispose(SensorsDataSource *self);
static DataSourceOps sensors_data_source_ops = {
    .frame = (DataSourceFrameFunc)sensors_data_source_frame,
    .dispose = (DataSourceDisposeFunc)sensors_data_source_dispose
};

SensorsDataSource *sensors_data_source_new(void)
{
    SensorsDataSource *self;

    self = calloc(1, sizeof(SensorsDataSource));
    if(self){
        if(!sensors_data_source_init(self)){
            free(self);
            return NULL;
        }
    }
    return self;
}

SensorsDataSource *sensors_data_source_init(SensorsDataSource *self)
{
    if(!data_source_init(DATA_SOURCE(self), &sensors_data_source_ops))
        return NULL;

    if(!jy61_init(&self->jy61_dev)){
        printf("Couldn't initialize device, bailing out\n");
        exit(EXIT_FAILURE);
    }

    jy61_start(&self->jy61_dev);

    DATA_SOURCE(self)->latitude = 45.215470;
    DATA_SOURCE(self)->longitude = 5.844828;
    DATA_SOURCE(self)->altitude = 718.267245;
    DATA_SOURCE(self)->heading = 43.698940;

    return self;
}

static SensorsDataSource *sensors_data_source_dispose(SensorsDataSource *self)
{
    jy61_dispose(&self->jy61_dev);
    return self;
}

static bool sensors_data_source_frame(SensorsDataSource *self, uint32_t dt)
{
    double roll, pitch, yaw;
    bool rv;

    if(dt != 0 && dt < (1000/25)) //One update per 1/25 second
        return false;

    jy61_get_attitude(&self->jy61_dev, &roll, &pitch, &yaw);

    DATA_SOURCE(self)->roll = roll;
    DATA_SOURCE(self)->pitch = pitch;
    /* Heading is not yaw and will be get using a magnetometer
     * */

    return true;
}

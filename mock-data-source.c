#include <stdlib.h>
#include "mock-data-source.h"

static bool mock_data_source_frame(MockDataSource *self, uint32_t dt);
static DataSourceOps mock_data_source_ops = {
    .frame = (DataSourceFrameFunc)mock_data_source_frame,
    .dispose = NULL
};


MockDataSource *mock_data_source_new(void)
{
    MockDataSource *self;

    self = calloc(1, sizeof(MockDataSource));
    if(self){
        if(!mock_data_source_init(self)){
            data_source_free(DATA_SOURCE(self));
            return NULL;
        }
    }
    return self;
}

MockDataSource *mock_data_source_init(MockDataSource *self)
{
    if(!data_source_init(DATA_SOURCE(self), &mock_data_source_ops))
        return NULL;
#if 0
    data_source_set_location(
        DATA_SOURCE(self), &(LocationData){
            .super.latitude = 45.215470,
            .super.longitude = 5.844828,
            .altitude = 718.267245
        }
    );

    data_source_set_attitude(
        DATA_SOURCE(self), &(AttitudeData){
            .roll = 0,
            .pitch = 0,
            .heading = 43.698940
        }
    );
#endif
    return self;
}


static bool mock_data_source_frame(MockDataSource *self, uint32_t dt)
{
    data_source_set_location(
        DATA_SOURCE(self), &(LocationData){
            .super.latitude = 45.215470,
            .super.longitude = 5.844828,
            .altitude = 718.267245
        }
    );

    data_source_set_attitude(
        DATA_SOURCE(self), &(AttitudeData){
            .roll = 0,
            .pitch = 0,
            .heading = 43.698940
        }
    );

    DATA_SOURCE(self)->has_fix = true;
    return true;
}



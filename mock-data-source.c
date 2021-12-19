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
            free(self);
            return NULL;
        }
    }
    return self;
}

MockDataSource *mock_data_source_init(MockDataSource *self)
{
    if(!data_source_init(DATA_SOURCE(self), &mock_data_source_ops))
        return NULL;

    DATA_SOURCE(self)->latitude = 45.215470;
    DATA_SOURCE(self)->longitude = 5.844828;
    DATA_SOURCE(self)->altitude = 718.267245;

    DATA_SOURCE(self)->heading = 43.698940;

    return self;
}


static bool mock_data_source_frame(MockDataSource *self, uint32_t dt)
{
    DATA_SOURCE(self)->latitude = 45.215470;

    return true;
}



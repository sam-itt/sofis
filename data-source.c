#include <stdio.h>
#include <stdarg.h>

#include "data-source.h"
#include "misc.h"

static DataSource *_datasource = NULL;

/*forward declarations of private functions*/
static bool get_listener_range(DataType type, uintf8_t *start, uintf8_t *limit);


DataSource *data_source_get_instance(void)
{
    return _datasource;
}

void data_source_set(DataSource *source)
{
    _datasource = source;
}


/**
 * @brief Allow a single object to register for several events at once.
 *
 * @param target The object that will receive the event
 * @param nevents Number of DataType/ValueListenerFunc couples that follows
 * @param ... couples of DataType/ValueListenerFunc
 *
 * @return The number of successfuly added listeners. A value different from
 * @param nevents indicates a failure after the return value nth listener
 */
size_t data_source_add_events_listener(DataSource *self, void *target,
                                           size_t nevents, ...)
{
    va_list args;
    int i;

    va_start(args, nevents);
    for(i = 0; i < nevents; i++){
        DataType type = va_arg(args, DataType);
        ValueListenerFunc func = va_arg(args, ValueListenerFunc);
        bool rv = data_source_add_listener(self, type,&(ValueListener){
            .callback = func,
            .target = target
        });
        if(!rv) break;
    }
    va_end(args);
    return i;
}

bool data_source_add_listener(DataSource *self, DataType type, ValueListener *listener)
{
    uintf8_t idx, limit;

    self = self ? self : data_source_get_instance();

    get_listener_range(type, &idx, &limit);
    if(self->nlisteners[type] == limit){
        printf(
            "Tried to add another location listener while %d limit has been reached."
            "Please increment the corresponding listeners limit\n", limit
        );
        return false;
    }
    self->listeners[idx+self->nlisteners[type]] = *listener;
    self->nlisteners[type]++;

    return true;
}


void data_source_print_listener_stats(DataSource *self)
{
    printf(
        "Current number of listeners:\n"
        "\tlocation: %zu\n"
        "\tattitude: %zu\n"
        "\tdynamics: %zu\n"
        "\tengine data: %zu\n",
        self->nlisteners[LOCATION_DATA],
        self->nlisteners[ATTITUDE_DATA],
        self->nlisteners[DYNAMICS_DATA],
        self->nlisteners[ENGINE_DATA]
    );
}

static void data_source_fire_listeners(DataSource *self, DataType type, void *param)
{
    uintf8_t idx, limit;

    self = self ? self : data_source_get_instance();

    get_listener_range(type, &idx, &limit);
    for(int i = idx; i < idx + self->nlisteners[type]; i++){
        self->listeners[i].callback(
            self->listeners[i].target,
            param
        );
    }
}

void data_source_set_location(DataSource *self, LocationData *location)
{
    self = self ? self : data_source_get_instance();

    if(location_equals(location, &self->location))
        return;

    data_source_fire_listeners(self, LOCATION_DATA, location);
    self->location = *location;
}

void data_source_set_attitude(DataSource *self, AttitudeData *attitude)
{
    self = self ? self : data_source_get_instance();

    if(attitude_equals(attitude, &self->attitude))
        return;

    data_source_fire_listeners(self, ATTITUDE_DATA, attitude);
    self->attitude = *attitude;
}

void data_source_set_dynamics(DataSource *self, DynamicsData *dynamics)
{
    self = self ? self : data_source_get_instance();

    if(dynamics_equals(dynamics, &self->dynamics))
        return;
    data_source_fire_listeners(self, DYNAMICS_DATA, dynamics);
    self->dynamics = *dynamics;
}


void data_source_set_engine_data(DataSource *self, EngineData *engine_data)
{
    self = self ? self : data_source_get_instance();

    if(engine_data_equals(engine_data, &self->engine_data))
        return;
    data_source_fire_listeners(self, ENGINE_DATA, engine_data);
    self->engine_data = *engine_data;
}

static bool get_listener_range(DataType type, uintf8_t *start, uintf8_t *limit)
{
    switch(type){
        case LOCATION_DATA:
            *start = 0;
            *limit = *start + MAX_LOCATION_LISTENERS;
            return true;
        case ATTITUDE_DATA:
            *start = MAX_LOCATION_LISTENERS;
            *limit = *start + MAX_ATTITUDE_LISTENERS;
            return true;
        case DYNAMICS_DATA:
            *start = MAX_LOCATION_LISTENERS+MAX_ATTITUDE_LISTENERS;
            *limit = *start + MAX_DYNAMICS_LISTENERS;
            return true;
        case ENGINE_DATA:
            *start = MAX_LOCATION_LISTENERS+MAX_ATTITUDE_LISTENERS+MAX_DYNAMICS_LISTENERS;
            *limit = *start + MAX_ENGINE_DATA_LISTENERS;
            return true;
        break;
        default:
            printf("CRIT: %s: bad type, problems ahead!\n",__FUNCTION__);
            return 0;
        break;
    };
}


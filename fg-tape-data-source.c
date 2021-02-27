#include <stdio.h>
#include <stdlib.h>

#include "data-source.h"
#include "fg-tape-data-source.h"

/*private struct*/
typedef struct{
    double latitude;
    double longitude;
    double altitude;
    float roll;
    float pitch;
    float heading;
    float slip_rad;
    float airspeed; //kts
    float vertical_speed; //vertical speed //feets per second

    float rpm;
    float fuel_flow;
    float oil_temp;
    float oil_press;
    float cht;
    float fuel_px;
    float fuel_qty;
}TapeRecord;



static bool fg_tape_data_source_frame(FGTapeDataSource *self, uint32_t dt);
static FGTapeDataSource *fg_tape_data_source_dispose(FGTapeDataSource *self);
static DataSourceOps fg_tape_data_souce_ops = {
    .frame = (DataSourceFrameFunc)fg_tape_data_source_frame,
    .dispose = (DataSourceDisposeFunc)fg_tape_data_source_dispose
};



FGTapeDataSource *fg_tape_data_source_new(char *filename, int start_pos)
{
    FGTapeDataSource *self;

    self = calloc(1, sizeof(FGTapeDataSource));
    if(self){
        if(!fg_tape_data_souce_init(self, filename, start_pos)){
            free(self);
            return NULL;
        }
    }
    return self;
}

FGTapeDataSource *fg_tape_data_souce_init(FGTapeDataSource *self, char *filename, int start_pos)
{
    int found;

    if(!data_source_init(DATA_SOURCE(self), &fg_tape_data_souce_ops))
        return NULL;

    self->tape = fg_tape_new_from_file(filename);
    if(!self->tape)
        return NULL;
//    fg_tape_dump(tape);
//
    found = fg_tape_get_signals(self->tape, self->signals,
        "/position[0]/latitude-deg[0]",
        "/position[0]/longitude-deg[0]",
        "/position[0]/altitude-ft[0]",
        "/orientation[0]/roll-deg[0]",
        "/orientation[0]/pitch-deg[0]",
        "/orientation[0]/heading-deg[0]",
        "/orientation[0]/side-slip-rad[0]",
        "/velocities[0]/airspeed-kt[0]",
	    "/velocities[0]/vertical-speed-fps[0]",
        "/engines[0]/engine[0]/rpm[0]",
        "/engines[0]/engine[0]/fuel-flow-gph[0]",
        "/engines[0]/engine[0]/oil-temperature-degf[0]",
        "/engines[0]/engine[0]/oil-pressure-psi[0]",
        "/engines[0]/engine[0]/cht-degf[0]",
        "/engines[0]/engine[0]/fuel-px-psi[0]",
        "/consumables[0]/fuel[0]/tank[0]/level-gal_us[0]",
        NULL
    );
    printf("TapeRecord: found %d out of %d signals\n",found, 16);

    self->position = start_pos * 1000; /*Starting position in the tape*/
    self->playing= true;

    return self;
}

static FGTapeDataSource *fg_tape_data_source_dispose(FGTapeDataSource *self)
{
    if(self->tape)
        fg_tape_free(self->tape);
    return self;
}

static bool fg_tape_data_source_frame(FGTapeDataSource *self, uint32_t dt)
{
    TapeRecord record;
    int rv;

    if(dt != 0 && dt < (1000/25)) //One update per 1/25 second
        return false;

    if(!self->playing)
        return false;

    self->position += dt;
    rv = fg_tape_get_data_at(self->tape, self->position / 1000.0, 16, self->signals, &record);
    if(rv <= 0)
        return false; /*Error or end of tape*/

    DATA_SOURCE(self)->latitude = record.latitude;
    DATA_SOURCE(self)->longitude = record.longitude;
    DATA_SOURCE(self)->altitude = record.altitude;

    DATA_SOURCE(self)->airspeed = record.airspeed;
    DATA_SOURCE(self)->vertical_speed = record.vertical_speed;
    DATA_SOURCE(self)->slip_rad = record.slip_rad;

    DATA_SOURCE(self)->roll = record.roll;
    DATA_SOURCE(self)->pitch = record.pitch;
    DATA_SOURCE(self)->heading = record.heading;

    DATA_SOURCE(self)->rpm = record.rpm;
    DATA_SOURCE(self)->fuel_flow = record.fuel_flow;
    DATA_SOURCE(self)->oil_temp = record.oil_temp;
    DATA_SOURCE(self)->oil_press = record.oil_press;
    DATA_SOURCE(self)->cht = record.cht;
    DATA_SOURCE(self)->fuel_px = record.fuel_px;
    DATA_SOURCE(self)->fuel_qty = record.fuel_qty;

    return true;
}

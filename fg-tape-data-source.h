#ifndef FG_TAPE_DATA_SOURCE_H
#define FG_TAPE_DATA_SOURCE_H
#include <stdint.h>
#include <stdbool.h>

#include "data-source.h"
#include "fg-tape.h"

typedef struct{
    DataSource super;

    FGTape *tape;
    FGTapeSignal signals[16];

    uint32_t position;
    bool playing;
}FGTapeDataSource;

FGTapeDataSource *fg_tape_data_source_new(char *filename, int start_pos);
FGTapeDataSource *fg_tape_data_souce_init(FGTapeDataSource *self, char *filename, int start_pos);


#endif /* FG_TAPE_DATA_SOURCE_H */

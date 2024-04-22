/* atapi.h
   Purpose: ATAPI routines */
#pragma once

#include "atadef.h"
#include "ide_state.h"
#include <stdint.h>

void atapiReadSector(struct ide_channel* state, uint64_t lba, 
                     size_t length, uint8_t* buffer);


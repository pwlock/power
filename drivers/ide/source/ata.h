/* ata.h
   Purpose: Routines for non-ATAPI devices */
#pragma once

#include "ide_state.h"

void ataReadSector(struct ide_channel* state, uint64_t lba, 
                     size_t length, uint8_t* buffer);
void ataWriteSector(struct ide_channel* state, uint64_t lba, 
                    const uint8_t* buffer);

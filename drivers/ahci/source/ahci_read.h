/* ahci_read.h
   Purpose: implementation of {read,write}Sector */
#pragma once

#include "ahci_state.h"

int ahataReadSector(struct ahci_device* device, uint64_t lba, 
                    uint8_t* restrict buffer, size_t length);

int ahatapiReadSector(struct ahci_device* device, uint64_t lba, 
                    uint8_t* restrict buffer, size_t length);

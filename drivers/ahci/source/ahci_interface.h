/* ahci_interface.h
   Purpose: Implementation of driver_disk{_device}_interface */
#pragma once

#include "ahci_state.h"

void ahciRegisterDriverInterface(struct ahci*);
void ahciRegisterDiskInterface(struct ahci_device*);

/* ahci_device.h
   Purpose: AHCI device */
#pragma once

#include "ahci_state.h"

/* Word 83 */
#define AHCI_IDENTIFY_48BIT_BIT (1 << 10)

void ahciDeviceConfigure(struct ahci_device* dev);

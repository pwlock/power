/* driver_kinterface.h
   Purpose: Kernel interface for 
      the driver to communicate with. */
#pragma once

#include "pci.h"
#include <stdint.h>

struct kdriver_manager
{
    struct pci_device* LoadReason;
    struct driver_info* Info;
};

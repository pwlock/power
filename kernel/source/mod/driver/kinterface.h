/* kinterface.c
   Purpose: Implementation of interfaces 
            specified in driver_kinterface.h */
#pragma once

#include "driver_kinterface.h"
#include "mod/driver/driver.h"
#include "pci.h"

struct kdriver_manager* modCreateDriverManager(struct driver_info*, struct pci_device*);

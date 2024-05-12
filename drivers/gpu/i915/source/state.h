/* state.h
   Purpose: GPU state keeping */
#pragma once

#include "pci.h"
#include "utils/vector.h"

struct i915
{
    volatile void* MioAddress; /*< Memory IO address */
    volatile void* GttAddress; /*< Graphics translation table address */
    volatile void* StolenAddress; /*< Stolen memory address */

    struct s_event* PchInterruptEvent;
    struct vector Pipes;
};

struct i915* i915CreateState(struct pci_device* gpu);
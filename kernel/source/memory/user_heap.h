/* user_heap.h
   Purpose: Routines that allocate 
        (page-aligned!) memory for the user. */
#pragma once

#include "config.h"
#include "um/syscall.h"

uint64_t syscVirtualMap(union sysc_regs* regs);
uint64_t syscVirtualUnmap(union sysc_regs* regs);

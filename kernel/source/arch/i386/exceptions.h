/* exceptions.h
   Purpose: CPU exception handling */
#pragma once

#include "arch/i386/idt.h"

void excInstallExceptionHandlers(pfn_idt_interrupt_t* restrict handlers);

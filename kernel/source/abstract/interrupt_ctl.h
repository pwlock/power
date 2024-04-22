/* interrupt_ctl.h
   Purpose: generic timer interface */
#pragma once

#include "abstract/interrupt_ctl_interface.h"

#define INT_CTL_TYPE_PIC 0x1
#define INT_CTL_TYPE_APIC 0x2

struct int_ctl;

struct int_ctl* intCtlCreateDefault();
struct int_ctl* intCtlGetDefault();

int intCtlGetControllerType();
void intCtlAckInterrupt();
void intCtlHandleInterrupt(int vector, int_handler_pfn_t ih, void* data);
void intCtlExecuteInterrupt(int vector);
uint64_t intCtlGetMsiAddress(uint64_t* data, int vector);

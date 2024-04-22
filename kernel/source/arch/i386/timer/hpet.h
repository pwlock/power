/* hpet.h
   Purpose: HPET timer implementation */
#pragma once

#include <stdint.h>

#include "abstract/timer_interface.h"
#include "acpi.h"

struct __attribute__((packed))
hpet_header
{
    struct acpi_system_desc_header Header;
    uint32_t EventBlockTimerId;
    struct acpi_address_structure Address;
    uint8_t TimerNumber;
    uint8_t PageProtectionOem;
};

struct hpet_timer
{
    struct timer Base;
    volatile uint64_t* BaseAddress;
    uint64_t TimerPeriod;
    int ApicLine;
    int TimerNumber;
};

struct hpet_timer* hpetCreateTimer();

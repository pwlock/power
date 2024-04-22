/* apict.h
   Purpose: The Local APIC timer */
#pragma once

#include "abstract/timer_interface.h"

#define ApicValue(a, V) a[LAPIC_REG(V)]

struct apic_timer
{
   struct timer Base;
};

struct apic_timer* apicCreateTimer();

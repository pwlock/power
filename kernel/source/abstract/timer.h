/* timer.h
   Purpose: generic timer interface */
#pragma once

#include <stddef.h>
#define ARRAY_LENGTH(A) sizeof(*A) / sizeof(A)

#define TIMER_TYPE_HPET 0x1
#define TIMER_TYPE_PIT  0x2

struct timer;

void tmCreateTimer();
struct timer* tmGetDefaultTimer();

/* The secondary timer inside the kernel
   is used for preempting to the scheduler.
   
   Unfortunately, the implementation of this
   routine is left as a job to the 
   architecture-specific port. */
void tmCreateSecondaryTimer();
struct timer* tmGetSecondaryTimer();

int tmGetType(struct timer*);

/* Returns a number in such that, when passed
   to SetReloadValue, makes the CPU stall for
   1ms. */
int tmGetMillisecondUnit(struct timer*);

void tmSetReloadValue(struct timer*, size_t units);
void tmDisable(struct timer*);
void tmEnable(struct timer*);

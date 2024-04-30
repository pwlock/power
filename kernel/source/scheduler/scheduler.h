/* scheduler.h
   Purpose: Preemptive task scheduler */
#pragma once

#include "arch/i386/idt.h"
#include "config.h"
#include "thread.h"

struct scheduler;

struct scheduler* schedCreate();
void schedThink(const struct idt_register_state* state);

struct thread* schedGetCurrentThread();
struct thread* schedGetThreadById(int id);
void schedSleep(int ticks);

void schedAddThread(struct thread*);
void schedRemoveThread(const struct thread*);

void schedEnable(bool flag);
bool schedIsEnabled();

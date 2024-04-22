/* synchronization.h 
   Purporse: Synchronization primitives */
#pragma once

#include "config.h"

struct s_event;
struct s_event* schedCreateEvent();
void schedDestroyEvent(struct s_event*);

void schedEventResetFlag(struct s_event*);
bool schedEventFlag(struct s_event*);
void schedEventRaise(struct s_event*);
void schedEventPause(struct s_event*);

struct s_mutex;
struct s_mutex* schedCreateMutex();
void schedDestroyMutex(struct s_mutex*);
void schedMutexAcquire(struct s_mutex*);
void schedMutexRelease(struct s_mutex*);

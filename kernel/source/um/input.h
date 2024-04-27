/* input.h
   Purpose: Input buffer implementation */
#pragma once

#include "config.h"
#include "power/input.h"

void inpCreateInputRing();
bool inpAddReader();
void inpRemoveReader();

/* pop event from ring buffer */
struct input_event inpReadEvent();

/* subFlags - flags to be sent to user. */
void inpSendInputEvent(int type, int value, int subFlags);

/* input.h
   Purpose: input definitions for user mode */
#pragma once

#include <stdint.h>

#define KEY_NO_POLLUTE
#include "input_code.h"

#define EV_INPUT_KEYPRESS 0x1
#define EV_INPUT_KEYRELEASE 0x2

struct input_event
{
    int Type;
    int Code;
    int Flags;

    uint64_t Reserved;
};

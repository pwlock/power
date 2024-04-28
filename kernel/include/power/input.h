/* input.h
   Purpose: input definitions for user mode */
#pragma once

#include "types.h"

#define KEY_NO_POLLUTE
#include "input_code.h"

#define EV_INPUT_KEYPRESS 0x1
#define EV_INPUT_KEYRELEASE 0x2

struct input_event
{
    __int32 Type;
    __int32 Code;
    __int32 Flags;

    __uint64 Reserved;
};

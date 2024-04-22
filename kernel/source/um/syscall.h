/* syscall.h
   Purpose: System calling */
#pragma once

#include <stdint.h>

union sysc_regs
{
    struct {
        uint64_t Rax, Rbx, Rdx, R8, R9;
    };

    struct {
        uint64_t Slot, Arg1, Arg2, Arg3, Arg4;
    };
};
/* fpstate.h
   Purpose: FXSAVE and FXRSTOR */
#pragma once

static inline int fpsGetBufferSize()
{
    return 512;
}

static inline int fpsGetBufferAlignment()
{
    return 16;
}

static inline void fpsSave(char* state)
{
    asm volatile("fxsave %0" : "=m"(*state));
}

static inline void fpsRestore(void* state)
{
    /* BROKEN. Not really a priority to fix,
       given the only use case for this instruction
       is written in assembly file. */
    asm volatile("fxrstor %0" :: "a"(state));
}

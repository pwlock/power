/* thread.h
   Purpose: the representation of a 
            parallel unit of execution,
            to be scheduled. */
#pragma once

#include "config.h"
#include "arch/i386/paging.h"
#include "arch/i386/idt.h"

#define THREAD_CREATE_USER_MODE 0x00000000001

#define THREAD_NOT_SLEEPING 0xFFFFFFFF /*< Thread suspended but not by Sleep. */

typedef void(*thread_execution_pfn_t)(void*);

struct __attribute__((packed))
thread_registers
{
    struct idt_interrupt_frame Pointers;
    struct idt_gp_register_state GeneralRegisters;
    uint16_t CodeSegment;
    uint16_t DataSegment;
    uint64_t Cr3;
    void* FloatingPointState;
};

struct thread_timing
{
    uint64_t LastCount;
    uint64_t ElapsedCount;
    unsigned int ResetQuantum;
    unsigned int CurrentQuantum;
};

struct __attribute__((packed))
thread
{
    struct thread_registers Registers;
    struct thread_timing Timing;
    struct process* Process;
    void* StackTop;
    void* KernelStack;
    int Identifier;
    bool Suspended;
    unsigned int SuspendedTicks;
};

struct thread_args
{
    address_space_t* AddressSpace;
    uint32_t Reserved; 
};

struct thread* schedCreateThread(thread_execution_pfn_t ep, void* data, int flags);
struct thread* schedCreateThreadEx(thread_execution_pfn_t ep, void* data, struct thread_args*, int flags);
void schedThreadYield(struct thread* restrict);
void schedFreeThread(struct thread* restrict);
void* threadAllocateStack(address_space_t*, size_t size);

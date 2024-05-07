#include "thread.h"
#include "arch/i386/fpstate.h"
#include "arch/i386/gdt.h"
#include "arch/i386/paging.h"
#include "config.h"
#include "memory/address.h"
#include "memory/physical.h"
#include "scheduler/scheduler.h"
#include "term/terminal.h"

#define USER_STACK_BEGIN 0x464F000000

static void threadFinish()
{
    schedRemoveThread(schedGetCurrentThread());
    while (true)
        asm volatile("sti; hlt");
}

struct thread* 
schedCreateThread(thread_execution_pfn_t ep, void* data, int flags)
{
    return schedCreateThreadEx(ep, data, NULL, flags);
}

struct thread* 
schedCreateThreadEx(thread_execution_pfn_t ep, void* data, struct thread_args* args, 
                    int flags)
{
    struct thread* th = mmAllocKernelObject(struct thread);
    memset(th, 0, sizeof(*th));

    uint64_t* address = !args ? NULL : args->AddressSpace;
    if (flags & THREAD_CREATE_USER_MODE) {
        th->StackTop = threadAllocateStack(address, 8192);
        th->KernelStack = threadAllocateStack(address, 4096);
    }
    else {
        th->StackTop = mmAllocKernel(8192) + 8192;
    }

    th->Registers.Pointers.Rsp = (uint64_t)th;
    th->Registers.Pointers.Rip = (uint64_t)ep;
    th->Registers.GeneralRegisters.Rdi = (uint64_t)data;
    th->Registers.CodeSegment = (flags & THREAD_CREATE_USER_MODE) ? USER_CODE_SEGMENT : KERNEL_CODE_SEGMENT;
    th->Registers.DataSegment = (flags & THREAD_CREATE_USER_MODE) ? USER_DATA_SEGMENT : KERNEL_DATA_SEGMENT;
    th->Registers.Cr3 = (uint64_t)address;
    if (fpsGetBufferSize() && flags & THREAD_CREATE_USER_MODE) {
        th->Registers.FloatingPointState = mmAlignedAlloc(fpsGetBufferSize(), fpsGetBufferAlignment());
        memset(th->Registers.FloatingPointState, 0, fpsGetBufferSize());
    }

    uint64_t* ptr = pgGetPhysicalAddress(address, (uint64_t)th->StackTop - sizeof(uint64_t));
    th->Registers.Pointers.Rsp = ((uint64_t)th->StackTop) - 8;
    ptr[0] = 0;
    
    th->Timing.ResetQuantum = 3;
    return th;
}

void schedFreeThread(struct thread* restrict thread)
{
    if (thread->KernelStack) {
        mmAlignedFree(thread->KernelStack, 4096);
    }

    mmAlignedFree(thread->Registers.FloatingPointState, 512);
    mmAlignedFree(thread->StackTop, 4096);
    mmFreeKernelObject(thread);
}

void schedThreadYield()
{
    struct thread* thread = schedGetCurrentThread();
    thread->Timing.CurrentQuantum = 1;
    return;       
}

void* threadAllocateStack(address_space_t* addrspace, size_t size)
{
    void* stk = mmAlignedAlloc(size, 4096);
    struct address_space_mgr* c = addrGetManagerForCr3(addrspace);
    uint64_t stackbegin = USER_STACK_BEGIN;
    while (!asmgrClaimPage(c, stackbegin, 2)) {
        stackbegin += (5 * 4096);
    }

    for (size_t i = 0; i < size; i += 4096) {
        pgMapPage(addrspace, (uint64_t)stk + i, stackbegin + i, PT_FLAG_WRITE | PT_FLAG_USER);
    }

    return PaAdd(stackbegin, size);
}

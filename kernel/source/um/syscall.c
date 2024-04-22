/* syscall.c
   Purpose: syscall handler */

#include "syscall.h"

#include "power/error.h"
#include "arch/i386/paging.h"
#include "config.h"
#include "memory/user_heap.h"
#include "term/terminal.h"
#include "um/ioctl.h"
#include "um/process.h"
#include <stddef.h>
#include <stdint.h>
#include "utils/string.h"
#include "vfs.h"

#include <power/system.h>

typedef uint64_t(*syscall_handler_pfn_t)(union sysc_regs*);

static uint64_t syscCloseHandle(union sysc_regs* regs) 
{
    struct handle* buffer = pcGetHandleById(pcGetCurrentProcess(), regs->Rbx);
    pcCloseHandle(pcGetCurrentProcess(), buffer);
    return 0;
}

static uint64_t syscGetCommandLine(union sysc_regs* regs) 
{
    __unused(regs);
    return (uint64_t)pcGetCurrentProcess()->CommandLine;
}

static syscall_handler_pfn_t handlers[] = {
    NULL,
    &syscOpenFile,
    &syscWriteFile,
    &syscReadFile,
    &syscCloseHandle,
    &syscGetCommandLine,
    NULL,
    &syscVirtualMap,
    &syscVirtualUnmap,
    &syscSetFilePointer,
    &syscIoControl,
};

uint64_t syscGenericHandler(union sysc_regs* regs) 
{
    if (!regs->Rax) {
        return -ERROR_NO_SYSCALL;
    }

    size_t tableLength = sizeof(handlers) / sizeof(syscall_handler_pfn_t);
    if (regs->Rax > tableLength) {
        return -ERROR_NO_SYSCALL;
    }

    syscall_handler_pfn_t pf = handlers[regs->Rax];
    if (!pf) {
        return -ERROR_NO_SYSCALL;
    }

    return pf(regs);
}
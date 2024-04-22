#include "exceptions.h"
#include "arch/i386/idt.h"
#include "arch/i386/paging.h"
#include "config.h"
#include "memory/physical.h"
#include "scheduler/scheduler.h"
#include "term/terminal.h"
#include <stdbool.h>

extern volatile bool hpetSleepFlag;

static void undefinedOpcode(struct idt_register_state* restrict st)
{
    trmLogfn("Invalid instruction. RIP=%p", st->PointerRegisters->Rip);
    asm volatile("cli; hlt");
}

static void generalProtectionFault(int error, struct idt_register_state* restrict st)
{
    trmLogfn("System protection fault. Code=%p, RIP=%p", error, st->PointerRegisters->Rip);
    asm volatile("cli; hlt");
}

static void pageFault(int error, struct idt_register_state* restrict st)
{
    uint64_t cr2;
    asm volatile("movq %%cr2, %0" :"=r"(cr2));

    if (mmIsPhysical((void*)cr2) && !(error & (1 << 3))) {
        pgMapPage(NULL, cr2, cr2, PT_FLAG_WRITE);
        return;
    }

    trmLogfn("Page Fault! RIP=%p, error=%p, CR2=%p", st->PointerRegisters->Rip, error, cr2);
    asm volatile("cli; hlt");
}

static void apicTimer(struct idt_register_state* restrict st)
{
    if (schedIsEnabled())
        schedThink(st);
}

static void hpetTimer(struct idt_register_state* restrict st)
{
    __unused(st);
    hpetSleepFlag = true;
}

void excInstallExceptionHandlers(pfn_idt_interrupt_t* restrict handlers)
{
    handlers[6] = (pfn_idt_interrupt_t)undefinedOpcode;
    handlers[13] = (pfn_idt_interrupt_t)generalProtectionFault;
    handlers[14] = (pfn_idt_interrupt_t)pageFault;
    handlers[32] = (pfn_idt_interrupt_t)apicTimer;
    handlers[33] = (pfn_idt_interrupt_t)hpetTimer;
}

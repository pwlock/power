#include "idt.h"

#include <stdbool.h>
#include "abstract/interrupt_ctl.h"
#include "arch/i386/exceptions.h"
#include "arch/i386/gdt.h"
#include "memory/physical.h"
#include "term/terminal.h"

static struct idt_encoded_entries entries;
extern void* __idtInterruptsTable[];

static void* idtInterruptData[IDT_INTERRUPT_COUNT];

extern void idtSpuriousHandler();

static void ignoredInterrupt()
{}

static struct idt_entry_encoded encodeEntry(void* pfn, int vector)
{
    struct idt_entry_encoded ret = {0};
    ret.OffsetLow = (uint64_t)pfn & 0xFFFF;
    ret.OffsetMid = ((uint64_t)pfn & 0xFFFF0000) >> 16;
    ret.OffsetHigh = ((uint64_t)pfn & 0xFFFFFFFF00000000) >> 32;
    ret.Ist = vector == 32 ? 2 : 1;
    ret.Selector = 3 * 8;
    ret.Flags = 0x8F;
    ret.Reserved = 0;
    if (vector > 31)
        ret.Flags = 0x8E;

    return ret;
}

static inline bool requiresErrorCode(int code)
{
    return code == 8                  /* Double fault */
        || (code >= 10 && code <= 14) /* #TS-#PF */
        || code == 17                 /* #AC */
        || code == 21                 /* #CP */
        || code == 29                 /* #VC */
        || code == 30;                /* SX */
}

void idtInit()
{
    for (int i = 0; i < IDT_INTERRUPT_COUNT; i++) {
        entries.Interrupts[i] = encodeEntry(__idtInterruptsTable[i], i);
    }

    entries.Interrupts[47] = encodeEntry(idtSpuriousHandler, 47);
    _64memset(entries.InterruptHandlers, (uint64_t)&ignoredInterrupt, 
              IDT_INTERRUPT_COUNT);
    
    excInstallExceptionHandlers(entries.InterruptHandlers);

    struct gdt_pointer pointer = { sizeof(entries.Interrupts) - 1, 
                                   (uintptr_t)&entries.Interrupts };
    asm volatile("lidt %0" :: "m"(pointer));
    asm volatile("sti");
}

void idtGenericHandler(struct idt_interrupt_frame* restrict frame, 
                       int code, int error, 
                       struct idt_gp_register_state* restrict state)
{
    struct idt_register_state gp = { frame, state };
    void* intptr = entries.InterruptHandlers[code];
    if (requiresErrorCode(code)) {
        ((pfn_idt_e_interrupt_t)intptr)(error, &gp);
        return;
    }

    if (code >= 34 && code <= 41) 
        intCtlExecuteInterrupt(code);
    
    ((pfn_idt_interrupt_t)intptr)(&gp, idtInterruptData[code]);

    if (code > 31)
        intCtlAckInterrupt();
}

void idtHandleInterrupt(int vector, pfn_idt_interrupt_t hand, void* data)
{
    entries.InterruptHandlers[vector] = hand;
    idtInterruptData[vector] = data;
}

uint32_t idtGetFreeVector(int priority)
{
    static int priorities[14] = { 34, 48, 64, 80, 96, 112, 128,
                                  144, 160, 176, 192, 208, 224, 240 };
    priority = __max(__min(priority, 16), 2);

    int* prioritybase = &priorities[priority - 2];
    int prp = *prioritybase;
    if (++prp / 16 != priority) {
        /* There are > 16 intrs inside this prio block. */
        return 0;
    }

    (*prioritybase)++;
    return prp;
}

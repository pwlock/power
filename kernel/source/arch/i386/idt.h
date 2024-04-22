/* idt.h
   Purpose: Interrupt handling */
#pragma once

#include <stdint.h>

#define IDT_INTERRUPT_COUNT 65

struct idt_interrupt_frame
{
    uint64_t Rip;
    uint64_t Cs;
    uint64_t Rflags;
    uint64_t Rsp;
    uint64_t Ss;
};

struct __attribute__((packed)) 
idt_gp_register_state
{
    uint64_t R15;
    uint64_t R14;
    uint64_t R13;
    uint64_t R12;
    uint64_t R11;
    uint64_t R10;
    uint64_t R9;
    uint64_t Rsi;
    uint64_t Rdi;
    uint64_t Rdx;
    uint64_t Rcx;
    uint64_t Rbx;
    uint64_t Rax;
    uint64_t R8;
    uint64_t Rbp;
};

struct idt_register_state
{
    struct idt_interrupt_frame* PointerRegisters;
    struct idt_gp_register_state* GeneralRegisters;
};

struct __attribute__((packed)) 
idt_entry_encoded
{
    uint16_t OffsetLow;
    uint16_t Selector;
    uint8_t Ist;
    uint8_t Flags;
    uint16_t OffsetMid;
    uint32_t OffsetHigh;
    uint32_t Reserved;
};

typedef void(*pfn_idt_interrupt_t)(struct idt_register_state* regs, void* data);
typedef void(*pfn_idt_e_interrupt_t)(int error, struct idt_register_state* regs);
struct idt_encoded_entries
{
    struct idt_entry_encoded Interrupts[IDT_INTERRUPT_COUNT];
    pfn_idt_interrupt_t InterruptHandlers[IDT_INTERRUPT_COUNT];
};

void idtInit();
void idtHandleInterrupt(int vector, pfn_idt_interrupt_t hand, void* data);
uint32_t idtGetFreeVector(int priority);

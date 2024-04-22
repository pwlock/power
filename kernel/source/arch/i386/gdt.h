/* gdt.h
   Purpose: Global Descriptor Table loading */
#pragma once

#include <stdint.h>

/* REMEMBED TO EDIT syscall install code (in fet.s) 
   if editing these macros*/
#define KERNEL_CODE_SEGMENT 0x18
#define KERNEL_DATA_SEGMENT 0x20
#define USER_CODE_SEGMENT 0x28
#define USER_DATA_SEGMENT 0x30

struct __attribute__((packed)) 
gdt_pointer
{
    uint16_t Size;
    uint64_t Pointer;
};

typedef struct
{
    uint32_t Base;
    uint16_t Limit;
    uint8_t AccessByte;
    uint8_t Flags;
} gdt_entry;

typedef struct __attribute__((packed))
{
    uint16_t Limit;
    uint16_t BaseLow16;
    uint8_t BaseMid8;
    uint8_t AccessByte;
    uint8_t Flags;
    uint8_t BaseHigh8;
} gdt_entry_encoded;

typedef struct  __attribute__((packed)) 
{
    gdt_entry_encoded Gdt;
    uint32_t BaseHigher32;
    uint32_t Reserved;
} gdt_tss_entry_encoded;

typedef struct
{
    gdt_entry_encoded Null;
    gdt_entry_encoded Code32Bit;
    gdt_entry_encoded Data32Bit;
    gdt_entry_encoded KernelCode64Bit;
    gdt_entry_encoded KernelData64Bit;
    gdt_entry_encoded UserCode64Bit;
    gdt_entry_encoded UserData64Bit;
    gdt_tss_entry_encoded Tss;
} gdt_entries;

struct __attribute__((packed)) 
tss
{
    uint32_t Reserved0;
    uint64_t Rsp[3];
    uint64_t Reserved1;
    uint64_t Ist[7];
    uint64_t Reserved2;
    uint16_t Reserved3;
    uint16_t Iopb;
};

void gdtInit();

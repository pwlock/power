#include "gdt.h"
#include "memory/physical.h"
#include "term/terminal.h"

static struct tss tss;
static char _ring0Stack[4096];
static char _interruptStack[2048];
static char _interrupt32Stack[2048];

static gdt_entries entries = {
    { 0, 0, 0, 0, 0, 0 },
    { 0xffff, 0, 0, 0x9a, 0xcf, 0 },
    { 0xffff, 0, 0, 0x92, 0xcf, 0 },
    { 0xffff, 0, 0, 0x9a, 0xa2, 0 },
    { 0xffff, 0, 0, 0x92, 0xa0, 0 },
    { 0xffff, 0, 0, 0xFA, 0x20, 0 },
    { 0xffff, 0, 0, 0xF2, 0xa2, 0 },
    { { 0, 0, 0, 0, 0, 0 }, 0, 0 }
};

extern void gdtReloadSegments();

void gdtInit()
{
    entries.Tss.Gdt.BaseLow16 = ((uint64_t)(&tss) & 0xFFFF);
    entries.Tss.Gdt.BaseMid8  = (((uint64_t)(&tss) >> 16) & 0xFF);
    entries.Tss.Gdt.BaseHigh8 = (((uint64_t)(&tss) >> 24) & 0xFF);
    entries.Tss.BaseHigher32 = (((uint64_t)(&tss) >> 32) & 0xFFFFFFFF);
    entries.Tss.Gdt.AccessByte = 0x89;
    entries.Tss.Gdt.Limit = sizeof(struct tss);
    entries.Tss.Reserved = 0;

    memset(&tss, 0, sizeof(struct tss));
    tss.Rsp[0] = (uint64_t)_ring0Stack;
    tss.Ist[0] = (uint64_t)_interruptStack;
    tss.Ist[1] = (uint64_t)_interrupt32Stack;
    struct gdt_pointer ptr = { sizeof(gdt_entries) - 1, (uint64_t)&entries };

    asm volatile("lgdt %0" ::"m"(ptr));
    gdtReloadSegments();
}

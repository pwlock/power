#include "state.h"
#include "arch/i386/paging.h"
#include "memory/physical.h"
#include "mmio_reg.h"
#include "pci.h"
#include "pipe.h"
#include "term/terminal.h"
#include "utils/vector.h"

static void interrupt(struct idt_register_state* regs, void* data)
{
    struct i915* gpu = data;
    trmLogfn("het!");
    i915Write(gpu, REG_DISPLAY_IIR, i915Read(gpu, REG_DISPLAY_IIR));
}

struct i915* i915CreateState(struct pci_device* gpu)
{
    struct i915* st = mmAllocKernelObject(struct i915);
    uint32_t bar0 = pciReadDoubleWordFromDevice(gpu, 0x10) & ~0xF;
    uint32_t bar2 = pciReadDoubleWordFromDevice(gpu, 0x18) & ~0xF;

    for (int i = 0; i < 2048; i++) {
        pgMapPage(NULL, bar0 + (i * 4096), bar0 + (i * 4096), PT_FLAG_WRITE | PT_FLAG_PCD);
    }
    
    for (int i = 0; i < 128; i++) {
        pgMapPage(NULL, bar2 + (i * 4096), bar2 + (i * 4096), PT_FLAG_WRITE | PT_FLAG_PCD);
    }

    st->MioAddress = (volatile void*)(uintptr_t)bar0;
    st->GttAddress = NULL; /* st->MioAddres + SIZE OF MMIO SPACE */
    st->StolenAddress = (volatile void*)(uintptr_t)bar2;
    st->Pipes = vectorCreate(2);
    
    for (int i = 0; i < 2; i++) {
        st->Pipes.Data[i] = ippCreatePipe(st, i);
    }

    int er = pciHandleMessageInterrupt(gpu, interrupt, st);
    trmLogfn("pciMSI=%i", er);
    i915Write(st, REG_DISPLAY_IMR, 
            i915Read(st, REG_DISPLAY_IMR) & ~PCH_DPY_INTERRUPT_BIT);
    i915Write(st, REG_DISPLAY_IER, 
            i915Read(st, REG_DISPLAY_IER) | PCH_DPY_INTERRUPT_BIT);

    return st;
}

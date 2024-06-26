#include "ahci_state.h"

#include "ahci_device.h"
#include "ahci_interface.h"
#include "ahcidef.h"
#include "arch/i386/idt.h"
#include "arch/i386/paging.h"

#include "config.h"
#include "memory/physical.h"
#include "pci.h"

#include "scheduler/synchronization.h"

#include "term/terminal.h"
#include "utils/vector.h"
#include <stddef.h>

static void ahciInterrupt(struct idt_register_state* reg, void* data)
{
    __unused(reg);

    struct ahci* ahc = data;
    volatile struct ahci_abar* br = ahc->Bar;

    for (size_t i = 0; i < ahc->Devices.Length; i++) {
        struct ahci_device* dev = ahc->Devices.Data[i];
        if (br->InterruptStatus & BIT(dev->PortIndex)) {
            dev->ErrorInterrupt = false;
            if (dev->Port->InterruptStatus & IS_INT_TFES_BIT)
                dev->ErrorInterrupt = true;

            if (dev->Port->InterruptStatus & IS_INT_PCS_BIT)
                dev->Port->SataError = PORT_SERR_DIAG_X_BIT;

            if (dev->Port->InterruptStatus & IS_INT_PRCS_BIT)
                dev->Port->SataError = PORT_SERR_DIAG_PHYRDY_CHANGE_BIT;

            dev->Port->InterruptStatus = dev->Port->InterruptStatus;
            br->InterruptStatus = BIT(dev->PortIndex);
            schedEventRaise(dev->Waiter);
        }
    }
}

static inline void generateDevices(struct ahci* ahci,
                                   volatile struct ahci_abar* abar)
{
    uint32_t iplp = abar->PortsImplemented;
    volatile struct ahci_port* prt = &abar->FirstPort;

    for (int i = 0; i < 32; i++) {
        if ((iplp & (1 << i))) {
            volatile struct ahci_port* p = &prt[i];
            if ((p->SataStatus & DEVICE_DETECT_MASK) 
             == DEVICE_DETECT_PRESENT_PHY_ESTABLISHED) {
                struct ahci_device* d = ahciCreateDevice(ahci, i, p);
                if (d && d->EmptyDevice)
                    vectorInsert(&ahci->EmptyDevices, d);
            }
        }
    }

    trmLogfn("EXIT from generateDevices");
}

struct ahci* ahciCreateState(struct pci_device* dev)
{
    uint32_t bar5;
    struct ahci* ac = mmAllocKernelObject(struct ahci);
    ac->Devices = vectorCreate(5);
    ac->EmptyDevices = vectorCreate(1);

    pciHandleMessageInterrupt(dev, ahciInterrupt, ac);
    bar5 = pciReadDoubleWordFromDevice(dev, 0x24) & ~0xFFF;
    pgAddGlobalPage(bar5, bar5, PT_FLAG_WRITE | PT_FLAG_PCD);

    volatile struct ahci_abar* ab = (struct ahci_abar*)(uint64_t)bar5;
    ac->Bar = ab;
    
    uint32_t iplp = ab->PortsImplemented;
    volatile struct ahci_port* prt = &ab->FirstPort;

    for (int i = 0; i < 32; i++) {
        if ((iplp & (1 << i))) {
            volatile struct ahci_port* p = &prt[i];
            if ((p->SataStatus & DEVICE_DETECT_MASK)
                == DEVICE_DETECT_PRESENT_PHY_ESTABLISHED) {
                ahciSetCommandEngine(p, false);
            }
        }
    }

    ab->GlobalHostCtl |= GHC_INT_ENABLE_BIT;
    generateDevices(ac, ab);
    ahciRegisterDriverInterface(ac);
    trmLogfn("successfully created ctl ahci");
    return ac;
}

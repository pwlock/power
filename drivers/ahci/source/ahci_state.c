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
#include "scsi.h"
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
        if (br->InterruptStatus & (1 << dev->PortIndex)) {
            dev->ErrorInterrupt = false;
            if (dev->Port->InterruptStatus & IS_INT_TFES_BIT) {
                dev->ErrorInterrupt = true;
            }

            dev->Port->InterruptStatus = dev->Port->InterruptStatus;
            br->InterruptStatus = (1 << dev->PortIndex);
            schedEventRaise(dev->Waiter);
        }
    }
}

static struct command_table* createCommandTable(int devt)
{
    struct command_table* tb = mmAlignedAlloc(sizeof(struct command_table), 128);
    memset(tb, 0, sizeof(struct command_table));

    void* buffer;
    int length = devt == AHCI_DEVTYPE_ATA ? 2 : 4;
    for (int i = 0; i < length; i++) {
        struct prdt* pd = &tb->PhysicalRegion[i];
        buffer = mmAlignedAlloc(512, 2);
        pd->DataBaseAddress = (uint64_t)buffer & 0xFFFFFFFF;
        pd->UpperDataBaseAddress = ((uint64_t)buffer) >> 32;
        pd->ByteCount = 512 - 1;
    }

    return tb;
}

static void fillCommandTable(int devt, struct command_table_header* ct)
{
    ct->LengthFlags = (2 << COMMANDHTBL_LENGTH_SHIFT);
    
    struct command_table* cmdt = createCommandTable(devt);
    ct->CommandTableAddress = (uint64_t)cmdt & 0xFFFFFFFF;
    ct->UpperCommandTableAddress = ((uint64_t)cmdt) >> 32;
}

static int devSignatureToType(volatile struct ahci_port* pt)
{
    switch (pt->SignatureData) {
    case SATA_SIG_ATA:
        return AHCI_DEVTYPE_ATA;
    case SATA_SIG_ATAPI:
        return AHCI_DEVTYPE_ATAPI;
    default:
        trmLogfn("unknown device signature %p", pt->SignatureData);
        return 0;
    }
}

static struct ahci_device* createDevice(struct ahci* ahc,
                                        int index,
                                        volatile struct ahci_port* pt)
{
    asm volatile("sti");
    struct ahci_device* dev = mmAllocKernelObject(struct ahci_device);
    dev->PortIndex = index;
    dev->Port = pt;
    dev->Waiter = schedCreateEvent();
    dev->Type = devSignatureToType(pt);

    struct command_table_header* cmd;
    cmd = mmAlignedAlloc(sizeof(struct command_table_header) * 2, 1024);

    fillCommandTable(dev->Type, &cmd[0]);
    fillCommandTable(dev->Type, &cmd[1]);

    ahciRegisterDiskInterface(dev);
    ahciSetCommandEngine(dev, false);
    pt->CommandListBase = (uint64_t)cmd & 0xFFFFFFFF;
    pt->UpperCommandListBase = (uint64_t)(cmd) >> 32;

    struct fis_received* fis = mmAlignedAlloc(sizeof(struct fis_received), 256);
    memset(fis, 0, sizeof(struct fis_received));
    pt->FisBase = (uint64_t)fis & 0xFFFFFFFF;
    pt->UpperFisBase = (uint64_t)(fis) >> 32;
    ahciSetCommandEngine(dev, true);
    vectorInsert(&ahc->Devices, dev);

    /* Be sure to only emit commands AFTER this IE write.
       Or else the kernel will halt!
    
    Enable D2H, PIO FIS, DMA FIS and Task Error. */
    pt->InterruptEnable |= 0b111 | (1 << 30);
    ahciDeviceConfigure(dev);
    return dev;
}

struct ahci* ahciCreateState(struct pci_device* dev)
{
    uint32_t bar5;
    volatile struct ahci_abar* ab;
    volatile struct ahci_port* prt;
    struct ahci* ac = mmAllocKernelObject(struct ahci);
    ac->Devices = vectorCreate(5);

    pciHandleMessageInterrupt(dev, ahciInterrupt, ac);
    bar5 = pciReadDoubleWordFromDevice(dev, 0x24) & ~0xFFF;
    pgAddGlobalPage(bar5, bar5, PT_FLAG_WRITE | PT_FLAG_PCD);
    ab = (struct ahci_abar*)(uint64_t)bar5;
    prt = &ab->FirstPort;
    ac->Bar = ab;

    ahciRegisterDriverInterface(ac);
    ab->GlobalHostCtl |= (1 << 1);
    uint32_t iplp = ab->PortsImplemented;

    for (int i = 0; i < 32; i++) {
        if ((iplp & (1 << i)) != 0) {
            volatile struct ahci_port* p = &prt[i];
            if ((p->SataStatus & 0b1111) == 0x3) {
                createDevice(ac, i, p);
            }
        }
    }

    return ac;
}

void ahciSetCommandEngine(struct ahci_device* dev, bool enb)
{
    volatile struct ahci_port* pt = dev->Port;

    if (enb) {
        pt->CommandStatus |= PORT_COMMAND_STATUS_FISR;
        pt->CommandStatus |= PORT_COMMAND_STATUS_ST;
        while (!(pt->CommandStatus & PORT_COMMAND_STATUS_CR))
            ;
        return;
    }

    pt->CommandStatus &= ~(PORT_COMMAND_STATUS_ST);
    pt->CommandStatus &= ~(PORT_COMMAND_STATUS_FISR);
    while (pt->CommandStatus & PORT_COMMAND_STATUS_CR)
        ;
}

const uint16_t* ahciIdentifyDevice(struct ahci_device* dev)
{
    volatile struct ahci_port* pt = dev->Port;
    struct command_table_header* th;
    struct command_table* cth;

    th = getAddressUpper(pt, CommandListBase);
    cth = getAddressUpper(th, CommandTableAddress);

    th->LengthFlags = (1 << COMMANDHTBL_LENGTH_SHIFT)
        | ((sizeof(struct fis_h2d) / sizeof(uint32_t)) << COMMANDHTBL_FIS_LENGTH_SHIFT)
        | COMMANDHTBL_PREFETCH_BIT;

    struct fis_h2d* id = (struct fis_h2d*)cth->CommandFis;
    id->Type = FIS_H2D_TYPE;
    id->Device = 0;
    id->MultiplierCommand = FIS_H2D_COMMAND_BIT;

    switch (dev->Type) {
    case AHCI_DEVTYPE_ATA:
        id->Command = 0xEC;
        break;
    case AHCI_DEVTYPE_ATAPI:
        id->Command = 0xA1; /* IDENTIFY PACKET */
        break;
    default:
        trmLogfn("Unknown device type %i. Ignoring", dev->Type);
        return NULL;
    }

    pt->CommandIssue = 1;
    schedEventPause(dev->Waiter);
    struct prdt* pra = cth->PhysicalRegion;
    return getAddressUpper(pra, DataBaseAddress);
}

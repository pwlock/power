#include "ahci_device.h"
#include "ahci_interface.h"
#include "ahci_state.h"
#include "ahcidef.h"
#include "memory/physical.h"
#include "scheduler/scheduler.h"
#include "scheduler/synchronization.h"
#include "scsi.h"
#include "term/terminal.h"

static uint64_t atapiGetMaxLba(struct ahci_device* dev)
{
    uint8_t commandBuffer[10] = {
        0x25, 0,
        0, 0, 0, 0,
        0, 0,
        0, 0
    };

    ahscSubmitCommand(dev, commandBuffer, 10);
    volatile struct ahci_port* pt = dev->Port;
    struct command_table_header* th;
    struct command_table* cth;

    th = getAddressUpper(pt, CommandListBase);
    cth = getAddressUpper(th, CommandTableAddress);
    uint64_t* dma = getAddressUpper(cth->PhysicalRegion, DataBaseAddress);
    int sense = ahscRequestSense(dev);
    if (dev->ErrorInterrupt 
     || sense == 0x23a00 /* MEDIUM NOT PRESENT */) {
        return 0;
    }

    return *dma;
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

void ahciDeviceConfigure(struct ahci_device* dev)
{
    const uint16_t* buffer = ahciIdentifyDevice(dev);
    dev->EmptyDevice = false;
    if (dev->Type == AHCI_DEVTYPE_ATA) { 
        uint64_t* maxlb = (uint64_t*)(&buffer[100]);
        dev->Info.MaxLba = *maxlb;
    }
    else if (dev->Type == AHCI_DEVTYPE_ATAPI) {
        ahscMakeReady(dev);

        dev->EmptyDevice = ahscTrayEmpty(dev);
        /* Word 100 is reserved in IDENTIFY PACKET */
        dev->Info.MaxLba = atapiGetMaxLba(dev);
    }

    {
        uint16_t w106 = buffer[106];
        dev->Info.BytesPerSector = dev->Type == AHCI_DEVTYPE_ATA ? 512 : 2048;
        if ((w106 & (1 << 14)) && !(w106 & (1 << 15)) && (w106 & (1 << 12))) {
            uint32_t* ssptr = (uint32_t*)(&buffer[116]);
            dev->Info.BytesPerSector = *ssptr * sizeof(uint16_t);
        }
    }

    {
        uint16_t w83 = buffer[83];
        dev->Info.Lba48Bit = w83 & AHCI_IDENTIFY_48BIT_BIT;
    }
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

static void createMemoryFields(struct ahci_device* dev)
{
    struct fis_received* fis;
    struct command_table_header* cmd;
    volatile struct ahci_port* pt = dev->Port;

    cmd = mmAlignedAlloc(sizeof(struct command_table_header) * 2, 1024);
    pt->CommandListBase = (uint64_t)cmd & 0xFFFFFFFF;
    pt->UpperCommandListBase = (uint64_t)(cmd) >> 32;

    fis = mmAlignedAlloc(sizeof(struct fis_received), 256);
    memset(fis, 0, sizeof(struct fis_received));
    pt->FisBase = (uint64_t)fis & 0xFFFFFFFF;
    pt->UpperFisBase = (uint64_t)(fis) >> 32;

    fillCommandTable(dev->Type, &cmd[0]);
    fillCommandTable(dev->Type, &cmd[1]);
}

struct ahci_device* ahciCreateDevice(struct ahci* ahc,
                                     int index,
                                     volatile struct ahci_port* pt)
{
    asm volatile("sti");
    struct ahci_device* dev = mmAllocKernelObject(struct ahci_device);
    dev->PortIndex = index;
    dev->Port = pt;
    dev->Waiter = schedCreateEvent();
    dev->Type = devSignatureToType(pt);
    dev->CommandSemaphore = schedCreateMutex();
    vectorInsert(&ahc->Devices, dev);

    ahciSetCommandEngine(dev, false);
    createMemoryFields(dev);
    ahciSetCommandEngine(dev, true);

    /* Be sure to only emit commands AFTER this IE write.
       Or else the kernel will halt!

    Enable D2H, PIO FIS, DMA FIS and Task Error. */
    pt->InterruptEnable |= 0b111 | (1 << 30);
    ahciDeviceConfigure(dev);
    ahciRegisterDiskInterface(dev);
    return dev;
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

void ahciPollDeviceThread(void *data)
{
    struct ahci* ahci = (struct ahci*)data;
    trmLogfn("hello!");
    while(true) {
        schedSleep(1000);
        trmLogfn("trigger!");

        for (size_t i = 0; i < ahci->Devices.Length; i++) {
           struct ahci_device* dev = ahci->Devices.Data[i];
            if (dev->Type != AHCI_DEVTYPE_ATAPI)
                continue;

            dev->EmptyDevice = ahscTrayEmpty(dev);
        }
    }
}

#include "ahci_device.h"
#include "abstract/timer.h"
#include "ahci_interface.h"
#include "ahci_state.h"
#include "ahcidef.h"
#include "memory/physical.h"
#include "power/error.h"
#include "scheduler/synchronization.h"
#include "scsi.h"
#include "term/terminal.h"

static void decodeAtaString(const uint16_t* data, char* output)
{
    const char* dd = (const char*)data;
    for (int i = 0; i < 40; i += 2) {
        output[i] = dd[i + 1];
        output[i + 1] = dd[i];
    }

    output[40] = 0;
}

static uint64_t atapiGetMaxLba(struct ahci_device* dev)
{
    uint8_t commandBuffer[10] = { 0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    ahscSubmitCommand(dev, commandBuffer, 10);
    volatile struct ahci_port* pt = dev->Port;
    struct command_table_header* th;
    struct command_table* cth;

    th = getAddressUpper(pt, CommandListBase);
    cth = getAddressUpper(th, CommandTableAddress);
    uint64_t* dma = getAddressUpper(cth->PhysicalRegion, DataBaseAddress);
    int sense = ahscRequestSense(dev);
    if (dev->ErrorInterrupt 
     || (sense >> 8) == 0x23a /* MEDIUM NOT PRESENT */) {
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
    } else if (dev->Type == AHCI_DEVTYPE_ATAPI) {
        dev->EmptyDevice = ahscTrayEmpty(dev);
        if (!dev->EmptyDevice)
            dev->Info.MaxLba = atapiGetMaxLba(dev);
    }
    
    {
        uint16_t w106 = buffer[106];
        dev->Info.BytesPerSector = dev->Type == AHCI_DEVTYPE_ATA ? 512 : 2048;
        if (ATA_IDENTIFY_SECTOR_SIZE_WORD_VALID(w106)) {
            uint32_t* ssptr = (uint32_t*)(&buffer[116]);
            dev->Info.BytesPerSector = *ssptr * sizeof(uint16_t);
        }
    }

    {
        uint16_t w83 = buffer[83];
        dev->Info.Lba48Bit = w83 & AHCI_IDENTIFY_48BIT_BIT;
    }

    decodeAtaString(&buffer[27], dev->Info.Model);
}

static struct command_table* createCommandTable(int devt)
{
    struct command_table* tb =
        mmAlignedAlloc(sizeof(struct command_table), 128);
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

    __barrier();
    return tb;
}

static void fillCommandTable(int devt, struct command_table_header* ct)
{
    ahciWrite(&ct->LengthFlags, (2 << COMMANDHTBL_LENGTH_SHIFT));

    struct command_table* cmdt = createCommandTable(devt);
    ahciWrite(&ct->CommandTableAddress, (uint64_t)cmdt & 0xFFFFFFFF);
    ahciWrite(&ct->UpperCommandTableAddress, ((uint64_t)cmdt) >> 32);
}

static void createMemoryFields(struct ahci_device* dev)
{
    struct fis_received* fis;
    struct command_table_header* cmd;
    volatile struct ahci_port* pt = dev->Port;

    cmd = mmAlignedAlloc(sizeof(struct command_table_header) * 2, 1024);
    pt->CommandListBase = (uint64_t)cmd & 0xFFFFFFFF;
    pt->UpperCommandListBase = (uint64_t)(cmd) >> 32;
    memset(cmd, 0, sizeof(*cmd));

    fis = mmAlignedAlloc(sizeof(struct fis_received), 256);
    pt->FisBase = (uint64_t)fis & 0xFFFFFFFF;
    pt->UpperFisBase = (uint64_t)(fis) >> 32;
    memset(fis, 0, sizeof(*fis));

    fillCommandTable(dev->Type, &cmd[0]);
    fillCommandTable(dev->Type, &cmd[1]);

    pt->CommandStatus = PORT_COMMAND_STATUS_FISR_BIT;
    while (!(pt->CommandStatus & PORT_COMMAND_STATUS_FIS_RUNNING_BIT))
        asm volatile("pause");
}

static void resetPort(volatile struct ahci_port* pt)
{
    struct timer* tm = tmGetDefaultTimer();
    uint64_t ms = tmGetMillisecondUnit(tm);
    uint32_t sctl;
    uint32_t prevCmd = ahciRead(&pt->CommandStatus) & ~PORT_SCTL_IPM_BITS;

    ahciSetCommandEngine(pt, false);
    ahciWrite(&pt->SataCtl, 0);
    tmSetReloadValue(tm, ms);

    sctl = PORT_SCTL_IPM_DISABLED | PORT_SCTL_DET_INIT;
    ahciWrite(&pt->SataCtl, sctl);

    sctl &= ~PORT_SCTL_DET_INIT;
    sctl |= PORT_SCTL_DET_NONE;
    tmSetReloadValue(tm, ms);
    ahciWrite(&pt->SataCtl, sctl);

    while ((ahciRead(&pt->SataStatus) & DEVICE_DETECT_MASK)
           != DEVICE_DETECT_PRESENT_PHY_ESTABLISHED) {
        asm volatile("pause");
        tmSetReloadValue(tm, ms);
    }

    ahciWrite(&pt->SataError, (unsigned int)~0);
    while (ahciRead(&pt->TaskFileData) & PORT_TASK_BSY_BIT) {
        asm volatile("pause");
        tmSetReloadValue(tm, ms * 2);
    }

    ahciWrite(&pt->CommandStatus, prevCmd);
}

static void startPort(volatile struct ahci_abar* abar,
                      volatile struct ahci_port* pt)
{
    uint32_t cmd = ahciRead(&pt->CommandStatus) & ~PORT_COMMAND_STATUS_ICC_BITS;
    if (ahciRead(&abar->HostCap) & CAP_SSS_BIT) {
        cmd |= PORT_COMMAND_STATUS_SUD_BIT;
        ahciWrite(&pt->CommandStatus, cmd);
    }

    ahciWrite(&pt->CommandStatus, cmd | PORT_COMMAND_STATUS_ICC_ACTIVE);
    int tt = ahciSetCommandEngine(pt, true);
    if (tt == -ERROR_TIMED_OUT) {
        resetPort(pt);
    }

    ahciWrite(&pt->SataCtl, 0x300);

    ahciWrite(&pt->InterruptStatus, ~0U);
    ahciWrite(&pt->SataError, ~0U);

    ahciWrite(&pt->InterruptEnable, ~0 /*(1 << 30) | (0b11111 << 26) | 0b111*/);
}

struct ahci_device* ahciCreateDevice(struct ahci* ahc, int index,
                                     volatile struct ahci_port* pt)
{
    asm volatile("sti");
    struct ahci_device* dev = mmAllocKernelObject(struct ahci_device);
    dev->PortIndex = index;
    dev->Port = pt;
    dev->Waiter = schedCreateEvent();
    dev->Type = devSignatureToType(pt);
    dev->CommandSemaphore = schedCreateMutex();
    if (!dev->Type) {
        trmLogfn("Discarding unknown AHCI PORT with sign %p",
                 pt->SignatureData);
        goto error;
    }

    vectorInsert(&ahc->Devices, dev);
    if (pt->CommandStatus & PORT_COMMAND_STATUS_NOT_IDLE_BITS) {
        struct timer* tm = tmGetDefaultTimer();

        ahciSetCommandEngine(pt, false);
        ahciWrite(&pt->CommandStatus, pt->CommandStatus & ~(PORT_COMMAND_STATUS_FISR_BIT));

        tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 500);
    }

    createMemoryFields(dev);
    startPort(ahc->Bar, pt);
    ahciDeviceConfigure(dev);
    ahciRegisterDiskInterface(dev);
    return dev;

error:
    mmFreeKernelObject(dev);
    return NULL;
}

int ahciSetCommandEngine(volatile struct ahci_port* pt, bool enb)
{
    static int timeout = 6;
    struct timer* tm = tmGetDefaultTimer();
    uint64_t ms = tmGetMillisecondUnit(tm);
    int attempts = 0;

    if (enb) {
        ahciWrite(&pt->CommandStatus, pt->CommandStatus | PORT_COMMAND_STATUS_ST_BIT);
        do {
            if (attempts > timeout) {
                return -ERROR_TIMED_OUT;
            }

            tmSetReloadValue(tm, ms * 20);
            attempts++;
        } while (!(pt->CommandStatus & PORT_COMMAND_STATUS_CR_BIT));

        return attempts - 1;
    }

    ahciWrite(&pt->CommandStatus, pt->CommandStatus & ~(PORT_COMMAND_STATUS_ST_BIT));
    do {
        if (attempts > timeout) {
            return -ERROR_TIMED_OUT;
        }

        tmSetReloadValue(tm, ms * 20);
        attempts++;
    } while (!(pt->CommandStatus & PORT_COMMAND_STATUS_CR_BIT));

    return attempts - 1;
}

const uint16_t* ahciIdentifyDevice(struct ahci_device* dev)
{
    volatile struct ahci_port* pt = dev->Port;
    struct command_table_header* th;
    struct command_table* cth;

    th = getAddressUpper(pt, CommandListBase);
    cth = getAddressUpper(th, CommandTableAddress);

    th->LengthFlags = (1 << COMMANDHTBL_LENGTH_SHIFT)
        | ((sizeof(struct fis_h2d) / sizeof(uint32_t))
           << COMMANDHTBL_FIS_LENGTH_SHIFT)
        | COMMANDHTBL_PREFETCH_BIT
        | COMMANDHTBL_CLEAR_BIT;

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

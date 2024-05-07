#include "ahci_device.h"
#include "power/error.h"
#include "scheduler/synchronization.h"
#include "term/terminal.h"
#include "ahci_read.h"
#include "ahci_state.h"
#include "ahcidef.h"
#include "scsi.h"
#include "memory/physical.h"

int ahataReadSector(struct ahci_device* dev, uint64_t lba, 
                    uint8_t* restrict buffer, size_t length)
{
    if (lba > dev->Info.MaxLba)
        return -ERROR_TOO_BIG;

    if (dev->Type != AHCI_DEVTYPE_ATA) {
        /* Should never happen. */
        return -ERROR_FORBIDDEN;
    }

    if (length > dev->Info.BytesPerSector) {
        length = dev->Info.BytesPerSector;
    }

    volatile struct ahci_port* pt = dev->Port;
    struct command_table_header* th;
    struct command_table* cth;

    th = getAddressUpper(pt, CommandListBase);
    cth = getAddressUpper(th, CommandTableAddress);

    th->LengthFlags = (2 << COMMANDHTBL_LENGTH_SHIFT) 
                    | COMMANDHTBL_PREFETCH_BIT 
                    | COMMANDHTBL_CLEAR_BIT
                    | ((sizeof(struct fis_h2d) / sizeof(uint32_t)) 
                        << COMMANDHTBL_FIS_LENGTH_SHIFT);

    struct fis_h2d* hd = (struct fis_h2d*)cth->CommandFis;
    memset(hd, 0, sizeof(*hd));

    hd->Command = 0xC8; /* READ DMA */
    if (lba > 0xFFFFFFF) {
        hd->Command = 0x25; /* READ DMA EXT (for 48-bit) */
        hd->Lba3 = (uint8_t)(lba >> 24);
        hd->Lba4 = (uint8_t)(lba >> 32);
        hd->Lba5 = (uint8_t)(lba >> 40);
    }

    hd->LbaLow = (uint8_t)(lba);
    hd->LbaMid = (uint8_t)(lba >> 8);
    hd->LbaHigh = (uint8_t)(lba >> 16);
    hd->Count = 1;

    schedEventResetFlag(dev->Waiter);

    pt->CommandIssue = 1;
    schedEventPause(dev->Waiter);

    if (dev->ErrorInterrupt) {
        ahciSetCommandEngine(pt, false);
        ahciSetCommandEngine(pt, true);
        return -ERROR_IO;
    }

    struct prdt* pd = cth->PhysicalRegion;

    char* db;
    int i = 0, mw;
    while (length > 0) {
        db = getAddressUpper((&pd[i]), DataBaseAddress);
        mw = __min(512, length);
        memcpy(buffer + (i * 512), db, mw);
        i++; length -= mw;
    }

    return 0;
}

int ahatapiReadSector(struct ahci_device* dev, uint64_t lba, 
                    uint8_t* restrict buffer, size_t length)
{
    if (lba > dev->Info.MaxLba) {
        return -ERROR_TOO_BIG;
    }

    if (dev->Type != AHCI_DEVTYPE_ATAPI) {
        /* Should never happen. */
        return -ERROR_FORBIDDEN;
    }

    if (dev->EmptyDevice)
        return -ERROR_NO_DEVICE;

    if (length > dev->Info.BytesPerSector) {
        length = dev->Info.BytesPerSector;
    }

    volatile struct ahci_port* pt = dev->Port;
    struct command_table_header* th;
    struct command_table* cth;

    th = getAddressUpper(pt, CommandListBase);
    cth = getAddressUpper(th, CommandTableAddress);

    volatile uint8_t cmd[12] = {
        0xA8, 0, /* READ (12) */
        ((uint8_t)(lba >> 0x18) & 0xFF),
        ((uint8_t)(lba >> 0x10) & 0xFF),
        ((uint8_t)(lba >> 0x08) & 0xFF),
        ((uint8_t)(lba) & 0xFF),
        0, 0, 0, 1,
        0, 0
    };

    ahscSubmitCommand(dev, cmd, 12);
    if (dev->ErrorInterrupt) {
        ahciSetCommandEngine(pt, false);
        ahciSetCommandEngine(pt, true);
        return -ERROR_IO;
    }

    struct prdt* pd = cth->PhysicalRegion;

    char* db;
    int i = 0, mw;
    while (length > 0) {
        db = getAddressUpper((&pd[i]), DataBaseAddress);
        mw = __min(512, length);
        memcpy(buffer + (i * 512), db, mw);
        i++; length -= mw;
    }

    return 0;
}

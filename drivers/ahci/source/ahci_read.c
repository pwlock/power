#include "power/error.h"
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

    volatile struct ahci_port* pt = dev->Port;
    struct command_table_header* th;
    struct command_table* cth;

    th = getAddressUpper(pt, CommandListBase);
    cth = getAddressUpper(th, CommandTableAddress);

    th->LengthFlags = (1 << COMMANDHTBL_LENGTH_SHIFT) 
                    | ((sizeof(struct fis_h2d) / sizeof(uint32_t)) << COMMANDHTBL_FIS_LENGTH_SHIFT)
                    | COMMANDHTBL_PREFETCH_BIT;
    struct fis_h2d* hd = (struct fis_h2d*)cth->CommandFis;
    memset(hd, 0, sizeof(*hd));

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
    struct prdt* pd = cth->PhysicalRegion;

    int i = 0;
    char* db;
    int mw;

    while (length > 0) {
        db = getAddressUpper((&pd[i]), DataBaseAddress);
        mw = __min(512, length);
        memcpy(buffer + (i * 512), db, mw);
        i++; length -= mw;
    }

    return 0;
}

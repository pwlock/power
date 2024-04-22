#include "ata.h"
#include "arch/i386/ports.h"
#include "atadef.h"
#include "ide_state.h"
#include "scheduler/synchronization.h"
#include "memory/physical.h"
#include "term/terminal.h"

/* For compatibility with ATAPI, we can only read a entire sector at once. */
void ataReadSector(struct ide_channel* device, uint64_t lba, 
                     size_t length, uint8_t* buffer)
{
    struct ide_ports* ports = device->Ports;

    schedEventResetFlag(ports->WriteEvent);
    ideSetCurrentChannel(device->Ports, device->Channel, false);
    // if (device->SupportsLba48)
    ideSetLba48(ports, 1);
    outb(ports->Io + ATA_REG_FEATURES, 0); /* PIO read */
    outb(ports->Io + ATA_REG_SECNT, 1);

    outb(ports->Io + ATA_REG_LBA_LOW, (uint8_t)lba);
    outb(ports->Io + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(ports->Io + ATA_REG_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ports->Io + ATA_REG_COMMAND, 0x20); /* READ SECTORS */

    ports->PendingOp = true;
    schedEventPause(ports->WriteEvent);
    repInl(ports->Io + ATA_REG_DATA, 
        (uint32_t*)ports->OutputBuffer, 512 / sizeof(uint32_t));
    memcpy(buffer, ports->OutputBuffer, length);
}

void ataWriteSector(struct ide_channel* device, uint64_t lba, 
                    const uint8_t* buffer)
{
    struct ide_ports* ports = device->Ports;
    schedEventResetFlag(ports->WriteEvent);
    ideSetCurrentChannel(ports, device->Channel, false);
    ideSetLba48(ports, 1);
    outb(ports->Io + ATA_REG_FEATURES, 0); /* PIO read */
    outb(ports->Io + ATA_REG_SECNT, 1);

    outb(ports->Io + ATA_REG_LBA_LOW, (uint8_t)lba);
    outb(ports->Io + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(ports->Io + ATA_REG_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ports->Io + ATA_REG_COMMAND, 0x30); /* WRITE SECTORS */

    ports->PendingOp = true;
    uint16_t* bu16 = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(ports->Io + ATA_REG_DATA, bu16[i]);
    }

    outb(ports->Io + ATA_REG_COMMAND, 0xE7); /* CACHE FLUSHs */
}

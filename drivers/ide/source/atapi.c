#include "atapi.h"
#include "arch/i386/ports.h"
#include "atadef.h"
#include "ide_state.h"
#include "scheduler/synchronization.h"
#include "memory/physical.h"
#include "term/terminal.h"

static void waitStatus(struct ide_ports* pt)
{
    uint8_t status;
    do {
        status = inb(pt->Io + ATA_REG_STATUS);
    } while (status & 0x80);
}

void atapiReadSector(struct ide_channel* device, uint64_t lba, 
                     size_t length, uint8_t* buffer)
{
    struct ide_ports* ports = device->Ports;
    volatile uint8_t cmd[12] = {
        0xA8, 0,
        ((uint8_t)(lba >> 0x18) & 0xFF),
        ((uint8_t)(lba >> 0x10) & 0xFF),
        ((uint8_t)(lba >> 0x08) & 0xFF),
        ((uint8_t)(lba) & 0xFF),
            0, 0, 0, 1,
        0, 0
    };
    volatile uint16_t* cmd16 = (volatile uint16_t*)cmd;

    schedEventResetFlag(ports->WriteEvent);
    ideSetCurrentChannel(device->Ports, device->Channel, false);
    outb(ports->Io + ATA_REG_FEATURES, 0);
    outb(ports->Io + ATA_REG_LBA_MID, 2048 & 0x00FF);
    outb(ports->Io + ATA_REG_LBA_HIGH, (2048 & 0xFF00) >> 8);

    outb(ports->Io + ATA_REG_COMMAND, 0xA0);
    for (size_t i = 0; i < sizeof(cmd) / 2; i++)
        outw(ports->Io + ATA_REG_DATA, cmd16[i]);

    ports->PendingOp = true;
    schedEventPause(ports->WriteEvent);
    waitStatus(ports);
    uint64_t readLength = 2048 / sizeof(uint32_t);
    repInl(ports->Io + ATA_REG_DATA, 
        (uint32_t*)ports->OutputBuffer, readLength);
        
    memcpy(buffer, ports->OutputBuffer, length);
}

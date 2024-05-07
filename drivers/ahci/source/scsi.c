#include "scsi.h"
#include "ahci_device.h"
#include "ahcidef.h"

#include "abstract/timer.h"
#include "scheduler/synchronization.h"
#include "term/terminal.h"
#include "memory/physical.h"

int ahscSubmitCommand(struct ahci_device* dev, volatile const uint8_t* buffer, int length)
{
    volatile struct ahci_port* pt = dev->Port;
    struct command_table_header* th;
    struct command_table* cth;

    th = getAddressUpper(pt, CommandListBase);
    cth = getAddressUpper(th, CommandTableAddress);

    memset(cth->CommandFis, 0, sizeof(cth->CommandFis));
    th->LengthFlags = (4 << COMMANDHTBL_LENGTH_SHIFT) 
                    | ((sizeof(struct fis_h2d) / sizeof(uint32_t)) << COMMANDHTBL_FIS_LENGTH_SHIFT)
                    | COMMANDHTBL_PREFETCH_BIT | COMMANDHTBL_ATAPI_BIT;
    memset(cth->AtapiCommand, 0, sizeof(cth->AtapiCommand));
    memcpy(cth->AtapiCommand, (const uint8_t*)buffer, length);

    struct fis_h2d* id = (struct fis_h2d*)cth->CommandFis;
    id->Type = FIS_H2D_TYPE;
    id->Device = 0;
    id->Features = 1;
    id->MultiplierCommand = FIS_H2D_COMMAND_BIT;

    id->Command = 0xA0;
    
    trmLogfn("submitting command = %p, ci=%p serr=%p is=%p cmd=%p", 
        buffer[0], pt->CommandIssue, pt->SataError, pt->InterruptStatus, 
        pt->CommandStatus & PORT_COMMAND_STATUS_CR_BIT);
        
    schedEventResetFlag(dev->Waiter);
    pt->CommandIssue = 1;
    schedEventPause(dev->Waiter);

    if (dev->ErrorInterrupt) {
        /* Restart command engine in order to clear PxCI. */
        ahciSetCommandEngine(dev->Port, false);
        ahciSetCommandEngine(dev->Port, true);
    }

    return 0;
}

void ahscMakeReady(struct ahci_device* dev)
{
    volatile uint8_t buffer[6] = {
        0x1B, 0,
        0, 0, (1 << 1) | 1, 
        0
    };

    ahscSubmitCommand(dev, buffer, 6);
}

int ahscRequestSense(struct ahci_device* dev)
{
    volatile uint8_t buffer[6] = {
        0x03, 0,
        0, 0, 252, 
        0
    };

    ahscSubmitCommand(dev, buffer, 6);
    volatile struct ahci_port* pt = dev->Port;
    struct command_table_header* th;
    struct command_table* cth;

    th = getAddressUpper(pt, CommandListBase);
    cth = getAddressUpper(th, CommandTableAddress);
    struct prdt* pr = cth->PhysicalRegion;
    uint8_t* pb = getAddressUpper(pr, DataBaseAddress);

    int sense = pb[2] & 0xF;
    int asc = pb[12];
    int ascq = pb[13];

    return (sense << 16) | (asc << 8) | (ascq);
}

bool ahscTrayEmpty(struct ahci_device* dev)
{
    uint8_t commandBuffer[10] = {
        0x25, 0,
        0, 0, 0, 0,
        0, 0,
        0, 0
    };

    ahscSubmitCommand(dev, commandBuffer, 10);
    int sense = ahscRequestSense(dev);
    return (sense >> 8) == 0x23a; /* MEDIUM NOT PRESENT */
}

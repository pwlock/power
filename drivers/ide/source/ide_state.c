#include "ide_state.h"
#include "abstract/timer.h"
#include "atadef.h"
#include "abstract/interrupt_ctl.h"
#include "arch/i386/ports.h"
#include "driver.h"
#include "ide_interface.h"
#include "memory/physical.h"
#include "pci.h"
#include "scheduler/synchronization.h"
#include "term/terminal.h"

static void ideInterrupt(void* d);

static int getSectorSize(struct ide_channel* chan, const uint16_t* restrict idspace)
{
    uint16_t logs = idspace[106 / sizeof(uint16_t)];
    if ((logs & (1 << 14)) 
     && !(logs & (1 << 15)) 
     && (logs & (1 << 12))) {
        uint16_t sizeLow = idspace[117 / sizeof(uint16_t)];
        uint16_t sizeHigh = idspace[118 / sizeof(uint16_t)];
        uint32_t join = (sizeHigh << 16) | sizeLow;
        return join;
    }

    /* Let's just guess */
    return chan->Type == IDE_TYPE_ATA ? 512 : 2048;
}

static void setupPorts(struct ide* ide, struct pci_device* device)
{
    uint16_t irqs[2] = { 14, 15 };
    uint32_t off8 = pciReadDoubleWordFromDevice(device, 0x8);
    uint8_t progif = (off8 >> 8 & 0xFF);
    uint32_t bar[5] = { pciReadDoubleWordFromDevice(device, 0x10),
                        pciReadDoubleWordFromDevice(device, 0x14),
                        pciReadDoubleWordFromDevice(device, 0x18),
                        pciReadDoubleWordFromDevice(device, 0x1C),
                        pciReadDoubleWordFromDevice(device, 0x20) };

    if (((progif & 0b11) == 0b11)) { 
        /* Primary device is in native mode and 
           native mode can be switched */
        off8 &= ~(1 << 8);
        ide->Ports[IDE_DEV_PRIMARY].Io = bar[0];
        ide->Ports[IDE_DEV_PRIMARY].Control = bar[1];
    } else if ((progif & 0b11) == 0b01) {
        /* Primary device is in native mode but
           cannot get out of it */
        trmLogfn("usage of IDE device in native mode is unsupported!");
        ide->Ports[IDE_DEV_PRIMARY].Io = bar[0];
        ide->Ports[IDE_DEV_PRIMARY].Control = bar[1];
        irqs[0] = pciReadDoubleWordFromDevice(device, 0x3C) & 0xFF;
    } else {
        /* Primary device is in compat mode */
        ide->Ports[IDE_DEV_PRIMARY].Io = IDE_PRIMARY_PORT;
        ide->Ports[IDE_DEV_PRIMARY].Control = IDE_PRIMARY_CTL_PORT;
    }

    /* The same stuff as above but with the secondary device */
    if (((progif & 0b1100) == 0b1100)) { 
        off8 &= ~(1 << 8);
        ide->Ports[IDE_DEV_SECONDARY].Io = bar[2];
        ide->Ports[IDE_DEV_SECONDARY].Control = bar[3];
    } else if ((progif & 0b1100) == 0b0100) {
        trmLogfn("usage of IDE device in native mode is unsupported! (secondary)");
        ide->Ports[IDE_DEV_SECONDARY].Io = bar[2];
        ide->Ports[IDE_DEV_SECONDARY].Control = bar[3];
        irqs[1] = pciReadDoubleWordFromDevice(device, 0x3C) & 0xFF;
    } else {
        ide->Ports[IDE_DEV_SECONDARY].Io = IDE_SECONDARY_PORT;
        ide->Ports[IDE_DEV_SECONDARY].Control = IDE_SECONDARY_CTL_PORT;
    }

    ide->Devices[0].Ports = &ide->Ports[IDE_DEV_PRIMARY];
    ide->Devices[1].Ports = &ide->Ports[IDE_DEV_PRIMARY];
    ide->Devices[2].Ports = &ide->Ports[IDE_DEV_SECONDARY];
    ide->Devices[3].Ports = &ide->Ports[IDE_DEV_SECONDARY];

    intCtlHandleInterrupt(irqs[0], ideInterrupt, &ide->Ports[0]);
    intCtlHandleInterrupt(irqs[1], ideInterrupt, &ide->Ports[1]);
}

struct ide* ideCreateState(struct pci_device* device)
{
    struct ide* state = mmAllocKernelObject(struct ide);
    uint16_t* idspace = mmAlignedAlloc(256 * sizeof(uint16_t), 1);

    memset(state, 0, sizeof(struct ide));
    setupPorts(state, device);
    registerIdeInterface(state);

    int i = 0;
    for (int d = 0; d < 2; d++) {
        struct ide_ports* port = &state->Ports[d];
        port->WriteEvent = schedCreateEvent();    
        port->OutputBuffer = mmAlignedAlloc(2048, 1);
        ideEnableInterrupts(port, 1);

        for (int c = 0; c < 2; c++) {
            struct ide_channel* dev = &state->Devices[i];
            dev->Valid = true;
            dev->Channel = c;
            dev->Device = d;
            ideSetCurrentChannel(port, c, true);
            ideEnableInterrupts(port, 0);

            outb(port->Io + ATA_REG_LBA_LOW, 0);
            outb(port->Io + ATA_REG_LBA_MID, 0);
            outb(port->Io + ATA_REG_LBA_HIGH, 0);
            outb(port->Io + ATA_REG_COMMAND, 0xEC);
            uint8_t status = inb(port->Io + ATA_REG_STATUS);
            if (!status) {
                dev->Valid = false;
                i++;
                continue;
            }

            tmSetReloadValue(tmGetDefaultTimer(), 3000);
            do {
                status = inb(port->Io + ATA_REG_STATUS);
            } while((status & 0x80) /* BSY */ 
                 || !((status & 0x1) /* ERR */
                   || (status & 0x8)) /* DRQ */);

            dev->Type = IDE_TYPE_ATA;
            if (status & 0x1) {
                uint8_t md = inb(port->Io + ATA_REG_LBA_MID);
                uint8_t hg = inb(port->Io + ATA_REG_LBA_HIGH);
                if (md == 0x14 && hg == 0xEB) {
                    dev->Type = IDE_TYPE_ATAPI;
                    outb(port->Io + ATA_REG_COMMAND, 0xA1);
                    do {
                        status = inb(port->Io + ATA_REG_STATUS);
                        trmLogfn("status atapi ident %p", status);
                    } while(!(status & (1 << 3)) /* DRQ */);
                }
                else {
                    i++;
                    dev->Valid = false;
                    trmLogfn("ATA %i failed IDENTIFY.", i);
                    continue;
                }
            }

            repInl(port->Io + ATA_REG_DATA, (uint32_t*)idspace, 512 / sizeof(uint32_t));

            { /* Model name discovery */
                uint8_t* rawids = (uint8_t*)idspace;
                for (int i = 0; i < 40; i += 2) {
                    dev->Model[i] = rawids[ATA_IDENT_MODEL + i + 1];
                    dev->Model[i + 1] = rawids[ATA_IDENT_MODEL + i];
                }
                dev->Model[39] = 0;
            }

            dev->SectorSize = getSectorSize(dev, idspace);
            if (dev->Type == IDE_TYPE_ATA)
                dev->SupportsLba48 = idspace[83] & (1 << 10);
            else
                dev->SupportsLba48 = false;

            ideEnableInterrupts(port, 1);
            registerIdeDeviceInterface(dev);
            state->DeviceCount++;
            i++;
        }

        /* RESET EVERYTHING or else we will die at the first read. */
        outb(port->Control + ATA_CTL_REG_DEVCTL, (1 << 2));
        tmSetReloadValue(tmGetDefaultTimer(), 100);
        outb(port->Control + ATA_CTL_REG_DEVCTL, 0);
    }

    mmAlignedFree(idspace, 256 * sizeof(uint16_t));
    return state;
}

void ideEnableInterrupts(struct ide_ports* device, int enbstate)
{
    uint8_t ctl = inb(device->Control + ATA_CTL_REG_DEVCTL);
    ctl |= enbstate ? 0 : (1 << 1);
    outb(device->Control, ctl);
}

void ideSetCurrentChannel(struct ide_ports* ide, int channel, bool force)
{
    if (ide->CurrentChannel == channel && !force)
        return;

    outb(ide->Io + ATA_REG_DRIVE, 0xA0 | channel << 4);
    for (int i = 0; i < 15; i++) {
        inb(ide->Io + ATA_REG_STATUS);
    }
    ide->CurrentChannel = channel;
}

void ideInterrupt(void* d)
{
    struct ide_ports* dev = (struct ide_ports*)d;
    if (dev->PendingOp) {
        schedEventRaise(dev->WriteEvent);
    }
}

void ideSetLba48(struct ide_ports* port, int lba)
{
    uint8_t ctl = inb(port->Io + ATA_REG_DRIVE);
    ctl |= lba ? (1 << 6) : 0;
    outb(port->Io + ATA_REG_DRIVE, ctl);
}

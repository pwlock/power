#include "ahci_device.h"
#include "ahci_state.h"
#include "ahcidef.h"
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
}

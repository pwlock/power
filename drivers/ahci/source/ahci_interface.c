#include "ahci_interface.h"
#include "ahci_read.h"
#include "ahci_state.h"
#include "driver.h"
#include "power/error.h"

static int getDeviceCount(struct driver_disk_interface* di)
{
    struct ahci* ah = (struct ahci*)di;
    return ah->Devices.Length;
}

static struct driver_disk_device_interface*
getDevice(struct driver_disk_interface* di, int index)
{
    struct ahci* ah = (struct ahci*)di;
    if ((size_t)index > ah->Devices.Length) {
        return NULL;
    }

    return ah->Devices.Data[index];
}

void ahciRegisterDriverInterface(struct ahci* ah)
{
    ah->Base.getDeviceCount = getDeviceCount;
    ah->Base.getMaxDeviceCount = getDeviceCount; /* For compatibility. */
    ah->Base.getDevice = getDevice;
}

/* ahci_device */
static int getSectorSize(struct driver_disk_device_interface* ddi)
{
    struct ahci_device* ah = (struct ahci_device*)ddi;
    return ah->Info.BytesPerSector;
}

static int readSector(struct driver_disk_device_interface* ddi, uint64_t lba, 
                      size_t length, uint8_t* restrict buffer)
{
    struct ahci_device* ah = (struct ahci_device*)ddi;
    if (ah->Type == AHCI_DEVTYPE_ATAPI)
        return ahatapiReadSector(ah, lba, buffer, length);
    else if (ah->Type == AHCI_DEVTYPE_ATA)
        return ahataReadSector(ah, lba, buffer, length);

    return -ERROR_INVALID_ARGUMENT;
}

void ahciRegisterDiskInterface(struct ahci_device* dev)
{
    dev->Base.getSectorSize = getSectorSize;
    dev->Base.readSector = readSector;
    dev->Base.writeSector = NULL;
}

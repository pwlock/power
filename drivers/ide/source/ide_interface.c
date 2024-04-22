#include "ide_interface.h"
#include "ata.h"
#include "atapi.h"
#include "driver.h"
#include "ide_state.h"

static struct driver_disk_device_interface* 
getDevice(struct driver_disk_interface* di, int device)
{
    struct ide* ide = (struct ide*)di;
    if (device > 4 || !ide->Devices[device].Valid) {
        return NULL;
    }

    return (struct driver_disk_device_interface*)&ide->Devices[device];
}

static int
getDeviceCount(struct driver_disk_interface* di)
{
    return ((struct ide*)di)->DeviceCount;
}

static int
getMaxDeviceCount(struct driver_disk_interface* di)
{
    return 4;
}

void registerIdeInterface(struct ide* ide)
{
    ide->Base.getDevice = getDevice;
    ide->Base.getDeviceCount = getDeviceCount;
    ide->Base.getMaxDeviceCount = getMaxDeviceCount;
}

static int readSector(struct driver_disk_device_interface* ddi, uint64_t lba, 
                      size_t length, uint8_t* restrict buffer)
{
    struct ide_channel* c = (struct ide_channel*)ddi;
    if (c->Type == IDE_TYPE_ATAPI)
        atapiReadSector((struct ide_channel*)ddi, lba, length, buffer);
    else 
        ataReadSector(c, lba, length, buffer);
    return 0;
}

static int getSectorSize(struct driver_disk_device_interface* ddi)
{
    struct ide_channel* c = (struct ide_channel*)ddi;
    return c->SectorSize;
}

static int writeSector(struct driver_disk_device_interface* ddi, uint64_t lba,
                       const uint8_t* buffer) 
{
    struct ide_channel* c = (struct ide_channel*)ddi;
    if (c->Type == IDE_TYPE_ATA) {
        ataWriteSector(c, lba, buffer);
        return 0;
    }

    return -1;
}

void registerIdeDeviceInterface(struct ide_channel* device)
{
    device->Base.readSector = readSector;
    device->Base.writeSector = writeSector;
    device->Base.getSectorSize = getSectorSize;
}

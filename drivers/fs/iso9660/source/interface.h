/* vfs.h
   Purpose: Registering of VFS interface */
#pragma once

#include "driver.h"
#include "isodef.h"
#include "vfs.h"

struct isofs
{
    struct driver_fs_interface Base;
    struct driver_disk_device_interface* Device;
    struct iso9660_primary_vol Volume;
};

struct mounted_isofs
{
    struct driver_mounted_fs_interface Base;
    struct iso9660_directory_record RootDirectory;
    struct driver_disk_device_interface* Device;
    uint8_t* ScratchBuffer;
};

void registerVfsInterface(struct isofs*);
struct mounted_isofs* createMountedIso(struct isofs* restrict iso, 
                                        struct driver_disk_device_interface* device,
                                        struct fs_mountpoint* mp);

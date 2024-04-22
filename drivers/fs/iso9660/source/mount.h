/* mount.h
   Purpose: Mounting API */
#pragma once

#include "driver.h"
#include "interface.h"
#include "vfs.h"

void isofsCreateFile(struct mounted_isofs*, 
                     const char* path, 
                     struct fs_file* outFile);

int isofsReadFile(struct mounted_isofs* mount,
                   uint8_t* buffer,
                   size_t size,
                   struct fs_file* outFile);

void isofsCloseFile(struct mounted_isofs* mount, struct fs_file* file);

struct driver_mounted_fs_interface* 
isofsMount(struct isofs* iso, 
           struct driver_disk_device_interface* dev, 
           uint64_t beginLba, struct fs_mountpoint* mp);

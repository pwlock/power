#include "interface.h"
#include "term/terminal.h"
#include "vfs.h"
#include "driver.h"
#include "memory/physical.h"
#include "mount.h"

/* isofs */

static struct driver_mounted_fs_interface*
mount(struct driver_fs_interface* interface, 
        struct driver_disk_device_interface* dev, 
        uint64_t beginLba, struct fs_mountpoint* mp)
{
    return isofsMount((struct isofs*)interface, dev, beginLba, mp);
}

void registerVfsInterface(struct isofs* iso)
{
    iso->Base.Name = "iso9660";
    iso->Base.mount = mount;
}

/* mounted_isofs */

static void createFile(struct driver_mounted_fs_interface* inter,
                       const char* name, struct fs_file* outFile)
{
    isofsCreateFile((struct mounted_isofs*)inter, name, outFile);
}

static int read(struct driver_mounted_fs_interface* inter, uint8_t* buffer,
                 size_t size, struct fs_file* file)
{
    return isofsReadFile((struct mounted_isofs*)inter, buffer, size, file);
}

static void closeFile(struct driver_mounted_fs_interface* inter, struct fs_file* file)
{
    isofsCloseFile((struct mounted_isofs*)inter, file);
}

struct mounted_isofs* 
createMountedIso(struct isofs* restrict iso, struct driver_disk_device_interface* device,
                 struct fs_mountpoint* mp)
{
    struct mounted_isofs* ret = mmAllocKernelObject(struct mounted_isofs);
    ret->Base.createFile = createFile;
    ret->Base.read = read;
    ret->Base.closeFile = closeFile;

    ret->RootDirectory = iso->Volume.RootDirectoryRecord;
    ret->ScratchBuffer = mmAllocKernel(2048);
    ret->Device = device;

    mp->BlockSize = 2048;
    memcpy(mp->VolumeName, iso->Volume.VolumeIdentifier, FS_VOLUME_NAME_MAX);
    return ret;
}

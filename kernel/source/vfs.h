/* vfs.h 
   Purpose: Definitions used by filesystems
            and the virtual file system */
#pragma once

#include "config.h"
#include "mod/driver/driver.h"
#include "um/syscall.h"

#define FS_VOLUME_NAME_MAX 32

#define FS_ERROR_OK        0
#define FS_ERROR_NOT_FOUND 1

struct fs_mountpoint
{
    struct fs_mountpoint* Next;

    char Letter[2]; /*< Up to 99 + 26 mount letters possible. */
    char VolumeName[FS_VOLUME_NAME_MAX];
    size_t BlockSize;
    struct driver_mounted_fs_interface* Ops;
    size_t ScratchBufferLength;
    void* ScratchBuffer;
};

struct fs_file /*< Represents a open file handle. */
{
    int Flags;
    size_t Size;
    struct fs_mountpoint* Mounted;
    int Bad; /*< If the driver function failed (Bad != FS_ERROR_OK), 
                  this entire struct is invalid. */

    size_t Offset;
    void* Reserved; /*< for FSes */
};

void vfsInit();

struct fs_mountpoint* vfsCreateFilesystem(const char* filesystem,
                                          uint64_t beginLba,
                                          struct driver_disk_device_interface* device);

size_t vfsGetPathComponent(const char* path);
struct fs_file* vfsOpenFile(const char* path, int flags);
void vfsCloseFile(struct fs_file* file);

size_t vfsReadFile(struct fs_file* file, uint8_t* buffer, size_t size);
uint64_t syscOpenFile(union sysc_regs* regs);
uint64_t syscReadFile(union sysc_regs* regs);
uint64_t syscWriteFile(union sysc_regs* regs);
uint64_t syscSetFilePointer(union sysc_regs* regs);
uint64_t syscMountVolume(union sysc_regs* regs);

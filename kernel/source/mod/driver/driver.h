/* driver.h
   Purpose: Definitions used by drivers */
#pragma once

#include "driver_kinterface.h"
#include "vfs.h"

#include <stdbool.h>
#include <stdint.h>

#define DRIVER_CLD_RELATIONSHIP_NONE 0
#define DRIVER_CLD_RELATIONSHIP_AND 1
#define DRIVER_CLD_RELATIONSHIP_OR  2

#define DRIVER_CLD_TYPE_CUSTOM_FUNC 1
#define DRIVER_CLD_TYPE_PCI         2

#define DRIVER_DONT_CARE -1

typedef struct driver_info*(*driver_query_pfn_t)();
typedef void(*driver_main_pfn_t)(struct kdriver_manager*);
typedef bool(*driver_should_load_pfn_t)();

struct driver_info
{
    const char* Name;
    void* Interface;
    driver_main_pfn_t EntryPoint;
    struct driver_conditional_loading* ConditionalLoading;
    int Role;
};

struct driver_conditional_loading
{
    int RelationshipWithPrevious;
    int ConditionalType;
    int HasNext;
    union {
        struct driver_cld_pci {
            uint16_t VendorId;
            uint16_t DeviceId;
            uint8_t Class;
            uint8_t Subclass;
            uint8_t Reserved;
        } Pci;

        driver_should_load_pfn_t shouldLoad;
    };
};

/* driver disk interface */
#define DRIVER_ROLE_DISK 0x1

struct driver_disk_interface
{
    struct driver_disk_device_interface* (*getDevice)(struct driver_disk_interface*, int device);
    int (*getDeviceCount)(struct driver_disk_interface*);
    int (*getMaxDeviceCount)(struct driver_disk_interface*);
};

struct driver_disk_device_interface
{
    int (*readSector)(struct driver_disk_device_interface*, uint64_t lba, 
                      size_t length, uint8_t* restrict buffer);
    int (*writeSector)(struct driver_disk_device_interface*, uint64_t lba,
                       const uint8_t* buffer);
    int (*getSectorSize)(struct driver_disk_device_interface*);
};

/* Filesystem driver */
#define DRIVER_ROLE_FILESYSTEM 0x2

typedef uint64_t lba_t;
struct driver_fs_interface
{
    const char* Name; /*< File system name (used in mount syscalls) */
    struct driver_mounted_fs_interface* (*mount)(struct driver_fs_interface*, 
            struct driver_disk_device_interface* dev, lba_t beginLba, 
            struct fs_mountpoint* mp);
};

struct driver_mounted_fs_interface
{
    /* Construct a file handle. */
    void (*createFile)(struct driver_mounted_fs_interface*,
                       const char* name, struct fs_file* outFile);
    int (*read)(struct driver_mounted_fs_interface*, uint8_t* buffer,
                 size_t size, struct fs_file* file);
    void (*closeFile)(struct driver_mounted_fs_interface*, struct fs_file* file);

    uintptr_t Reserved[2];
};

/* Input driver */
#define DRIVER_ROLE_INPUT 0x3

/* Display driver */
#define DRIVER_ROLE_DISPLAY 0x5

/* ioctl.h
   Purpose: ioctl definitions 
        (especially when DEVICE param is NULL) */
#pragma once

#include "power/types.h"

#ifndef _POWER_IOCTL_H
#define _POWER_IOCTL_H

/* Disk IOCTLs */
#define IOCTL_GET_DISKS 1
#define IOCTL_OPEN_DISK 2
#define IOCTL_CLOSE_DISK 3
#define IOCTL_SEARCH_PARTITION 4

struct ioctl_get_disk
{
    __uint64 Length;
    __int32* Data;
};

union ioctl_uuid
{
    struct {
        __uint32 Data1;
        __uint16 Data2;
        __uint16 Data3;
        __uchar Data4[2];
        __uchar Data5[6];
    };
    __uchar Raw[16];
};

struct ioctl_search_partition
{
    int Disk;
    union ioctl_uuid Identifier;
};

/* Input IOCTLs */
#define IOCTL_GET_INPUT_BUFFER 10

#endif

/* ioctl.h
   Purpose: ioctl definitions 
        (especially when DEVICE param is NULL) */
#pragma once

#include <stdint.h>

#ifndef _POWER_IOCTL_H
#define _POWER_IOCTL_H

/* Disk IOCTLs */
#define IOCTL_GET_DISKS 1
#define IOCTL_OPEN_DISK 2
#define IOCTL_CLOSE_DISK 3
#define IOCTL_SEARCH_PARTITION 4

struct ioctl_get_disk
{
    unsigned long Length;
    int* Data;
};

union ioctl_uuid
{
    struct {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint8_t Data4[2];
        uint8_t Data5[6];
    };
    unsigned char Raw[16];
};

typedef int DEVICE; /*< Device handle */ 
struct ioctl_search_partition
{
    int Disk;
    union ioctl_uuid Identifier;
};

/* Input IOCTLs */
#define IOCTL_GET_INPUT_BUFFER 10

#endif

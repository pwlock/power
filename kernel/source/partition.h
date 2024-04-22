/* partition.h
   Purpose: disk partition handler */
#pragma once

#include "config.h"
#include "mod/driver/driver.h"
#include "power/ioctl.h"
#include "utils/vector.h"

typedef union ioctl_uuid uuid_t;

struct partition
{
    uuid_t Identifier;
    uint64_t BeginLba;
    uint64_t EndLba;
};

struct __attribute__((packed))
mbr_partition
{
    uint8_t Status;
    uint8_t HeadStart;
    uint16_t CylSectorStart;
    uint8_t PartitionType;
    uint8_t HeadEnd;
    uint16_t CylSectorEnd;
    uint32_t RelativeSector;
    uint32_t TotalSectors;
};

struct mbr_bootstrp
{
    uint8_t Reserved[446]; /*< Bootstrap code or something else*/
    struct mbr_partition Partitions[4];
    uint16_t Signature;
};

struct gpt_header
{
    char Signature[8];
    uint32_t Revision;
    uint32_t HeaderLength;
    uint32_t Checksum;
    uint32_t Reserved0;
    uint64_t CurrentHeaderLba;
    uint64_t SecondaryHeaderLba;
    uint64_t FirstPartitionLba;
    uint64_t LastPartitionLba;
    uuid_t Uid;
    uint64_t PartitionArrayBegin;
    uint32_t PartitionArrayCount;
    uint32_t PartitionArrayEntrySize;
    uint32_t PartitionArrayChecksum;
    uint32_t Reserved1;
};

struct gpt_partition
{
    uuid_t Type;
    uuid_t Uid;
    uint64_t FirstLba;
    uint64_t LastLba;
    uint64_t Attribute;
    char Name[72];
};

struct pt_disk
{
    int DeviceIndex; /*< index passed to driver getDevice */
    struct driver_disk_device_interface* Ops;
    struct vector Partitions;
};

void partInit(struct driver_disk_interface* di);
struct vector
partGetPartitions(struct driver_disk_device_interface* interf);
bool uuidCompare(union ioctl_uuid* lhs, union ioctl_uuid* rhs);
struct vector* partGetDisks();

/* isodef.h
   Purpose: ISO 9660 FS definitions */
#pragma once

#include <stdint.h>

union uint16_lsb_msb_t
{
    uint32_t Both;
    struct {
        uint16_t Little;
        uint16_t Big;
    };
};

union uint32_lsb_msb_t
{
    uint64_t Both;
    struct {
        uint32_t Little;
        uint32_t Big;
    };
};

struct __attribute__((packed)) 
iso9660_timestamp
{
    uint8_t Year; /*< Since 1900 */
    uint8_t Month;
    uint8_t Day;
    uint8_t Hour;
    uint8_t Minute;
    uint8_t Second;
    int8_t TimezoneOffset;
};

struct __attribute__((packed)) 
iso9660_directory_record
{
    uint8_t Size;
    uint8_t ExtendedAttribRecordSize;
    union uint32_lsb_msb_t ExtentLba;
    union uint32_lsb_msb_t DataLength;
    struct iso9660_timestamp RecordTimestamp;
    uint8_t Flags;
    uint8_t InterleavedFilesUnitSize;
    uint8_t InterleavedGap;
    union uint16_lsb_msb_t VolumeSequenceNumber;
    uint8_t FileIdentifierLength;
};

#define sizeofrec(R) R->FileIdentifierLength % 2 == 0 \
    ? sizeof(typeof(*(R))) + R->FileIdentifierLength + 1 \
    : sizeof(typeof(*(R))) + R->FileIdentifierLength
#define recname(R) PaAdd(R, sizeof(typeof(*(R))))

struct __attribute__((packed)) iso9660_primary_vol
{
    uint8_t TypeCode;
    char StandardIdentifier[5];
    uint8_t Version;
    uint8_t Unused0;

    char SystemIdentifier[32];
    char VolumeIdentifier[32];
    uint64_t Unused1;

    union uint32_lsb_msb_t VolumeSpaceSize;
    uint8_t Unused2[32];

    union uint16_lsb_msb_t VolumeSetSize;
    union uint16_lsb_msb_t VolumeSequenceNumber;
    union uint16_lsb_msb_t LogicalBlockSize;
    union uint32_lsb_msb_t PathTableSize;

    uint32_t LPathTableLocation;
    uint32_t OptionalLPathTableLocation;
    uint32_t BPathTableLocation;            /*< BIG ENDIAN  */
    uint32_t OptionalBPathTableLocation;    /*< BIG ENDIAN */

    struct iso9660_directory_record RootDirectoryRecord;
    uint8_t Unused3; /*< The Root Directory Identifier */
};

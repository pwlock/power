/* susp.h
   Purpose: Support for ISO9660 System Use Sharing Protocol */
#pragma once

#include "isodef.h"

struct __attribute__((packed)) 
susp_sue_header
{
    uint8_t Signature[2];
    uint8_t Length;
    uint8_t EntryVersion;
};

struct __attribute__((packed))
susp_alternate_name
{
    struct susp_sue_header Header;
    uint8_t Flags;
    /* The file name here... */
};

struct susp_new_state
{
    int FileNameLength;
    const char* FileName;
};

struct susp_new_state suspHandleEntry(struct iso9660_directory_record* record);

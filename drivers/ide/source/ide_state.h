/* ide_state.h
   Purpose: IDE state management */
#pragma once

#include "driver.h"
#include "pci.h"
#include "scheduler/synchronization.h"
#include <stdint.h>

#define IDE_PORTS_SIZE 2

#define IDE_CHAN_MASTER 0
#define IDE_CHAN_SLAVE 1

#define IDE_DEV_PRIMARY 0
#define IDE_DEV_SECONDARY 1

#define IDE_PRIMARY_PORT 0x1F0
#define IDE_PRIMARY_CTL_PORT 0x376
#define IDE_SECONDARY_PORT   0x170
#define IDE_SECONDARY_CTL_PORT 0x376

struct ide_ports
{
    struct s_event* WriteEvent;
    uint8_t* OutputBuffer;
    int CurrentChannel;
    uint16_t Control;
    uint16_t Io;
    bool PendingOp;
};

#define IDE_TYPE_ATA 1
#define IDE_TYPE_ATAPI 2

struct ide_channel
{
    struct driver_disk_device_interface Base;
    char Model[40];
    struct ide_ports* Ports;
    uint8_t Device;
    uint8_t Channel;
    uint8_t SupportsLba48;
    uint8_t Valid;

    int SectorSize;
    int Type;
    uint8_t CurrentChannel;
};

struct ide 
{
    struct driver_disk_interface Base;
    struct ide_ports Ports[IDE_PORTS_SIZE];
    struct ide_channel Devices[4];
    int DeviceCount;
    int CurrentDevice;
};

struct ide* ideCreateState(struct pci_device* device);
void ideSetCurrentChannel(struct ide_ports*, int channel, bool force);
void ideEnableInterrupts(struct ide_ports*, int enbstate);
void ideSetLba48(struct ide_ports*, int lba);

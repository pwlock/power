/* ahci_state.h
   Purpose: AHCI state management */
#pragma once

#include "ahcidef.h"
#include "driver.h"
#include "pci.h"
#include "utils/vector.h"

#define AHCI_DEVTYPE_ATA 1
#define AHCI_DEVTYPE_ATAPI 2

struct ahci_device_info
{
    char Model[40];
    uint64_t MaxLba;
    uint16_t BytesPerSector;

    bool Lba48Bit; /*< Support for 48-bit LBAs */
};

struct ahci_device
{
    struct driver_disk_device_interface Base;

    bool ErrorInterrupt; /*< TFES interrupt triggered */
    bool EmptyDevice;    /*< Nothing inside tray */
    int PortIndex;
    int Type;
    struct ahci_device_info Info;
    volatile struct ahci_port* Port;
    struct s_event* Waiter;
};

struct ahci /*< Driver state */
{
    struct driver_disk_interface Base;
    struct vector Devices;
    volatile struct ahci_abar* Bar;
};

struct ahci* ahciCreateState(struct pci_device* dev);
void ahciSetCommandEngine(struct ahci_device*, bool enb);
const uint16_t* ahciIdentifyDevice(struct ahci_device*);

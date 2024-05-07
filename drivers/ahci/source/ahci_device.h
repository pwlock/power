/* ahci_device.h
   Purpose: AHCI device */
#pragma once

#include "ahci_state.h"

/* Word 83 */
#define AHCI_IDENTIFY_48BIT_BIT (1 << 10)

struct ahci_device* ahciCreateDevice(struct ahci* ahc,
                                     int index,
                                     volatile struct ahci_port* pt);
void ahciDeviceConfigure(struct ahci_device* dev);
void ahciPollDeviceThread(void* data);

int ahciSetCommandEngine(volatile struct ahci_port*, bool enb);
const uint16_t* ahciIdentifyDevice(struct ahci_device*);

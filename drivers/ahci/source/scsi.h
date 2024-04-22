/* scsi.h 
   Purpose: SCSI (ATAPI) support */
#pragma once

#include "ahci_device.h"
#include "ahci_state.h"

#define SCSI_ASC_LOGICAL_UNIT_NOT_READY 0x04
#define SCSI_ASCQ_

/* Returns the command slot used. */
int ahscSubmitCommand(struct ahci_device* dev, 
                      volatile const uint8_t* buffer, int length);

/* Test the device */
void ahscMakeReady(struct ahci_device* dev);

int ahscRequestSense(struct ahci_device* dev);

bool ahscTrayEmpty(struct ahci_device* dev);

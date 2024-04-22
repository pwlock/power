/* atadef.h
   Purpose: Definitions for ATA code */
#pragma once

#define ATA_REG_DATA 0
#define ATA_REG_ERROR 1
#define ATA_REG_FEATURES ATA_REG_ERROR
#define ATA_REG_SECNT 2
#define ATA_REG_LBA_LOW 3
#define ATA_REG_LBA_MID 4
#define ATA_REG_LBA_HIGH 5
#define ATA_REG_DRIVE    6
#define ATA_REG_STATUS   7
#define ATA_REG_COMMAND  ATA_REG_STATUS

#define ATA_CTL_REG_STATUS 0
#define ATA_CTL_REG_DEVCTL ATA_CTL_REG_STATUS
#define ATA_CTL_REG_DRIVE_ADDRESS 1

#define ATA_IDENT_MODEL 23 * sizeof(uint16_t)

/* ahcidef.h
   Purpose: AHCI structures */
#pragma once

#include <stdint.h>

#define BIT(X) (1 << (X))

#define SATA_SIG_ATA 0x00000101
#define SATA_SIG_ATAPI 0xEB140101
#define SATA_SIG_SEMB 0xC33C0101
#define SATA_SIG_PM 0x96690101

#define GHC_INT_ENABLE_BIT (1 << 1)
#define GHC_HBA_RESET (1 << 0)

#define CAP_SSS_BIT (1 << 27) /*< Staggered spin-up support */

#define DEVICE_DETECT_MASK 0xF
#define DEVICE_DETECT_PRESENT_PHY_ESTABLISHED 0x3

#define FIS_H2D_TYPE 0x27
#define FIS_D2H_TYPE 0x34
#define FIS_DMA_ACTIVATE_TYPE 0x39
#define FIS_DMA_SETUP_TYPE 0x41
#define FIS_DATA_TYPE 0x46
#define FIS_BIST_TYPE 0x58
#define FIS_PIO_SETUP_TYPE 0x5F
#define FIS_DEV_BITS_TYPE 0xA1

#define FIS_H2D_COMMAND_BIT (1 << 7)
#define FIS_D2H_INTERRUPT_BIT (1 << 1)

#define COMMANDHTBL_FIS_LENGTH_SHIFT 0
#define COMMANDHTBL_LENGTH_SHIFT 16
#define COMMANDHTBL_PREFETCH_BIT (1 << 7)
#define COMMANDHTBL_CLEAR_BIT (1 << 10)
#define COMMANDHTBL_ATAPI_BIT (1 << 5)

#define PORT_COMMAND_STATUS_ICC_SHIFT (28)
#define PORT_COMMAND_STATUS_ICC_BITS (0xF << PORT_COMMAND_STATUS_ICC_SHIFT)
#define PORT_COMMAND_STATUS_ICC_ACTIVE (1 << PORT_COMMAND_STATUS_ICC_SHIFT)

#define PORT_COMMAND_STATUS_CPD_BIT (1 << 20)
#define PORT_COMMAND_STATUS_CR_BIT (1 << 15)
#define PORT_COMMAND_STATUS_FIS_RUNNING_BIT (1 << 14)
#define PORT_COMMAND_STATUS_FISR_BIT (1 << 4)
#define PORT_COMMAND_STATUS_POD_BIT (1 << 2)
#define PORT_COMMAND_STATUS_SUD_BIT (1 << 1)
#define PORT_COMMAND_STATUS_ST_BIT (1 << 0)

#define PORT_COMMAND_STATUS_NOT_IDLE_BITS                                      \
    (PORT_COMMAND_STATUS_ST_BIT | PORT_COMMAND_STATUS_CR_BIT                   \
     | PORT_COMMAND_STATUS_FIS_RUNNING_BIT | PORT_COMMAND_STATUS_FISR_BIT)

#define PORT_SCTL_DET_NONE 0
#define PORT_SCTL_DET_INIT 1

#define PORT_SCTL_IPM_SHIFT 8
#define PORT_SCTL_IPM_BITS (0xF << PORT_SCTL_IPM_SHIFT)
#define PORT_SCTL_IPM_DISABLED (0x3 << PORT_SCTL_IPM_SHIFT)

#define PORT_SERR_DIAG_X_BIT (1 << 26)
#define PORT_SERR_DIAG_PHYRDY_CHANGE_BIT (1 << 16)

#define PORT_TASK_DRQ_BIT (1 << 3)
#define PORT_TASK_BSY_BIT (1 << 7)
#define PORT_TASK_ERR_SHIFT 8

#define PORT_TASK_READY_BITS                                                   \
    (0xFF << PORT_TASK_ERR_SHIFT) | PORT_TASK_DRQ_BIT | PORT_TASK_BSY_BIT

#define IS_INT_TFES_BIT (1 << 30)
#define IS_INT_PRCS_BIT (1 << 22)
#define IS_INT_PCS_BIT (1 << 6)

#define UPPER_ADDRESS_SHIFT 32

/* ATA command set does not specify what these bits actually mean. */
#define ATA_IDENTIFY_SECTOR_SIZE_WORD_VALID(W)                                 \
    (W & (1 << 14)) && !(W & (1 << 15)) && (W & (1 << 12))

#define getAddressUpper(O, f)                                                  \
    (void*)((uint64_t)(O)->f | (uint64_t)(O->Upper##f) << UPPER_ADDRESS_SHIFT);

struct ahci_port {
    uint32_t CommandListBase; /*< Must be 1024-byte aligned*/
    uint32_t UpperCommandListBase;
    uint32_t FisBase; /*< Must be 256-byte aligned */
    uint32_t UpperFisBase;
    uint32_t InterruptStatus;
    uint32_t InterruptEnable;
    uint32_t CommandStatus;
    uint32_t Reserved0;
    uint32_t TaskFileData;
    uint32_t SignatureData;
    uint32_t SataStatus;
    uint32_t SataCtl;
    uint32_t SataError;
    uint32_t SataActive;
    uint32_t CommandIssue;
    uint32_t SataNotification;
    uint32_t SwitchCtl;
    uint32_t DeviceSleep;

    uint8_t Reserved1[39];
    uint32_t VendorSpecific[4];
};

struct ahci_abar /*< Data found at AHCI ABAR */
{
    /* Generic Host Ctl. */
    uint32_t HostCap;
    uint32_t GlobalHostCtl;
    uint32_t InterruptStatus;
    uint32_t PortsImplemented;
    uint32_t Version;
    uint32_t ComCplCtl;
    uint32_t ComCplPorts;
    uint32_t EnclosureLoc;
    uint32_t EnclosureCtl;
    uint32_t CapExtended;
    uint32_t BiosHandoff;

    uint8_t Reserved0[51];
    uint8_t Reserved1[63]; /*< for NVMHCI */
    uint8_t Reserved2[95]; /*< for Vendor */

    struct ahci_port FirstPort;
};

/* TODO: Verify if this is the correct struct
         against SATA specs */
struct __attribute__((packed)) fis_h2d /*< Host-to-device.  */
{
    uint8_t Type; /*< Always FIS_H2D_TYPE. */
    uint8_t MultiplierCommand; /*< PM and control or command*/

    /* Registers */
    uint8_t Command;
    uint8_t Features;

    uint8_t LbaLow; /*< LBA first byte */
    uint8_t LbaMid; /*< Second byte */
    uint8_t LbaHigh; /*< Third byte */
    uint8_t Device;

    uint8_t Lba3, Lba4, Lba5; /*< LBA 4th, 5th and 6th byte */
    uint8_t UpperFeatures;

    uint8_t Count;
    uint8_t UpperCount;
    uint8_t IsochronousCommandCpl;
    uint8_t Control;

    uint32_t Reserved;
};

struct fis_d2h /*< Device-to-host. */
{
    uint8_t Type; /*< Always FIS_D2H_TYPE. */

    uint8_t MultiplierInterrupt; /*< PM and interrupt bit. */
    uint8_t Status;
    uint8_t Error;

    uint8_t LbaLow;
    uint8_t LbaMid;
    uint8_t LbaHigh;
    uint8_t Device;

    uint8_t Lba3, Lba4, Lba5;
    uint8_t Reserved;

    uint8_t Count;
    uint8_t UpperCount;
    uint16_t Reserved2;

    uint32_t Reserved3;
};

struct fis_pio_setup /*< About to send/receive PIO data. */
{
    uint8_t Type; /*< Always FIS_PIO_SETUP_TYPE. */
    uint8_t MultiplierDirection; /*< PM, data direction and interrupt bit. */
    uint8_t Status;
    uint8_t Error;

    uint8_t LbaLow;
    uint8_t LbaMid;
    uint8_t LbaHigh;
    uint8_t Device;

    uint8_t Lba3, Lba4, Lba5;
    uint8_t Reserved;

    uint8_t Count;
    uint8_t UpperCount;
    uint8_t Reserved2;
    uint8_t NewStatus; /*< New value of status register. */

    uint16_t TransferCount;
    uint16_t Reserved3;
};

struct __attribute__((packed)) fis_dma_setup {
    uint8_t Type; /*< Always FIS_DMA_SETUP_TYPE. */
    uint8_t MultiplierInterrupt; /*< PM, data direction,
                                     interrupt bit and auto-activate. */
    uint16_t Reserved0;

    uint64_t DmaBufferId;

    uint32_t Reserved1;

    uint32_t DmaBufferOffset;
    uint32_t TransferCount;
    uint32_t Reserved2;
};

struct __attribute__((packed)) fis_received {
    struct fis_dma_setup DmaSetup;
    uint8_t Reserved0[4];

    struct fis_pio_setup PioSetup;
    uint8_t Reserved1[12];

    struct fis_d2h D2h;
    uint8_t Reserved2[4];

    uint64_t SetDeviceBitsFis;
    uint8_t Unknown[64];

    uint8_t Reserved3[95];
};

struct command_table_header {
    uint32_t LengthFlags; /*< Physical regions length + Flags */
    volatile uint32_t WriteByteCount; /*< Currently written bytes length */
    uint32_t CommandTableAddress; /*< Must be 128-byte aligned. */
    uint32_t UpperCommandTableAddress;

    uint32_t Reserved[4];
};

struct prdt {
    uint32_t DataBaseAddress; /*< Must be 2-byte aligned. */
    uint32_t UpperDataBaseAddress;
    uint32_t Reserved;
    uint32_t ByteCount; /*< Must be half the actual length
                            and no bigger than 4M (= 2M in memory)*/
};

struct command_table {
    uint8_t CommandFis[64];
    uint8_t AtapiCommand[16];
    uint8_t Reserved[48];
    struct prdt PhysicalRegion[4];
};

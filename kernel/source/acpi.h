/* acpi.h
   Purpose: ACPI tables support */
#pragma once

#include "config.h"

struct __attribute__((packed)) 
acpi_address_structure
{
    uint8_t AddressSpaceId;   /* 0 - system memory, 1 - system I/0 */
    uint8_t RegisterBitWidth;
    uint8_t RegisterBitOffset;
    uint8_t Reserved;
    uint64_t Address;
};

struct __attribute__((packed))
acpi_rsdp
{
    uint8_t Signature[8];
    uint8_t Checksum;
    uint8_t OemId[6];
    uint8_t Revision;
    uint32_t RsdtAddress;

    /* Only when revision >= 2 */
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t Reserved[3];
};

struct __attribute__((packed))
acpi_system_desc_header 
{
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    uint8_t OemId[6];
    uint8_t OemTableId[8];
    uint32_t OemRevision;
    uint32_t CreatorId;
    uint32_t CreatorRevision;
};

struct __attribute__((packed))
acpi_madt
{
    struct acpi_system_desc_header Header;
    uint32_t LocalApicAddress;
    uint32_t Flags; /* Bit 0 set = PIC 8259 is present */
};

struct __attribute__((packed))
acpi_fadt
{
    struct acpi_system_desc_header Header;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;

    uint8_t Reserved;
 
    uint8_t PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;
    uint32_t SMI_CommandPort;
    uint8_t AcpiEnable;
    uint8_t AcpiDisable;
    uint8_t S4BIOS_REQ;
    uint8_t PSTATE_Control;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t PM1EventLength;
    uint8_t PM1ControlLength;
    uint8_t PM2ControlLength;
    uint8_t PMTimerLength;
    uint8_t GPE0Length;
    uint8_t GPE1Length;
    uint8_t GPE1Base;
    uint8_t CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t DutyOffset;
    uint8_t DutyWidth;
    uint8_t DayAlarm;
    uint8_t MonthAlarm;
    uint8_t Century;

    uint16_t BootArchitectureFlags;
 
    uint8_t Reserved2;
    uint32_t Flags;
 
    struct acpi_address_structure ResetReg;
 
    uint8_t ResetValue;
    uint8_t Reserved3[3];

    uint64_t X_FirmwareControl;
    uint64_t X_Dsdt;
 
    struct acpi_address_structure X_PM1aEventBlock;
    struct acpi_address_structure X_PM1bEventBlock;
    struct acpi_address_structure X_PM1aControlBlock;
    struct acpi_address_structure X_PM1bControlBlock;
    struct acpi_address_structure X_PM2ControlBlock;
    struct acpi_address_structure X_PMTimerBlock;
    struct acpi_address_structure X_GPE0Block;
    struct acpi_address_structure X_GPE1Block;
};

struct acpi_system_desc_header* acpiGetRootTable(bool* wasRsdt);
struct acpi_system_desc_header* acpiFindTable(const char* restrict name);

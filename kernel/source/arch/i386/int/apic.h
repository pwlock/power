/* apic.h
   Purpose: Impl of int_ctl using APIC */
#pragma once

#include "abstract/interrupt_ctl_interface.h"
#include <stdint.h>

#define LAPIC_REG(R) (R) / sizeof(uint32_t)

/*v MADT entries v*/
struct madt_entry_header
{
    uint8_t EntryType;
    uint8_t RecordLength;
};

#define MADT_PROCESSOR_LOCAL_APIC 0
struct madt_entry_processor_local_apic
{
    struct madt_entry_header Header;
    uint8_t AcpiProcessorId;
    uint8_t ApicId;
    uint32_t Flags;
};

#define MADT_IO_APIC 1
struct madt_entry_io_apic
{
    struct madt_entry_header Header;
    uint8_t IoApicId;
    uint8_t Reserved;
    uint32_t IoApicAddress;
    uint32_t GsiBase;
};

#define MADT_IO_APIC_INT_SOURCE_OVERRIDE 2
struct madt_entry_io_apic_int_source_override
{
    struct madt_entry_header Header;
    uint8_t BusSource;
    uint8_t IrqSource;
    uint32_t GlobalSystemInterrupt;
    uint16_t Flags;
};

#define MADT_ENTRY_LOCAL_APIC_NMI 4
struct madt_entry_local_apic_nmi
{
    struct madt_entry_header Header;
    uint8_t AcpiProcessorId;
    uint16_t Flags;
    uint8_t Lint; /* 0 or 1 */
};

#define MADT_ENTRY_LOCAL_APIC_ADDR_OVERRIDE 5
struct madt_entry_local_apic_addr_override
{
    struct madt_entry_header Header;
    uint8_t Reserved;
    uint64_t LocalApicAddress;
};

struct apic_int_ctl
{
    struct int_ctl Base;

    void(*handleInterruptIo)(struct apic_int_ctl*, int entry, int vector);
    
    volatile uint32_t* IoApicAddress;
    uint32_t GlobalSystemBase;
};

struct apic_io_redir
{
    uint8_t Vector;
    uint16_t DeliveryMode;
    uint16_t DestinationMode;
    uint16_t DeliveryStatus;
    uint16_t PinPolarity;
    uint16_t RemoteIrr;
    uint16_t TriggerMode;
    uint16_t Mask;
    uint64_t Destination;
};

struct int_h_data
{
    void* Data;
    int_handler_pfn_t Handler;
    uint16_t Vector;
};

struct apic_int_ctl* apicCtlCreate();
int apicGetCurrentCpuId();
volatile uint32_t* apicGetLocalAddress();

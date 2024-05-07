#include "apic.h"
#include "abstract/interrupt_ctl.h"
#include "abstract/interrupt_ctl_interface.h"
#include "acpi.h"
#include "arch/i386/idt.h"
#include "arch/i386/paging.h"
#include "config.h"
#include "memory/physical.h"
#include "term/terminal.h"
#include "utils/vector.h"
#include <stdbool.h>

#define IoValue(A, r) A[0] = r; A[4]

static volatile uint32_t* lapicAddress = NULL;
static struct vector interruptHandlers;

static int isVectorFree(struct int_ctl*, int vector);
static int getControllerType(struct int_ctl*);
static void ackInterrupt(struct int_ctl*);
static void handleInterruptIo(struct apic_int_ctl* this, int entry, int vector);
static void handleInterrupt(struct int_ctl*, int vector, int_handler_pfn_t ih, void *data);
static void executeInterrupt(struct int_ctl*, int vector);
static uint64_t getMsiAddress(struct int_ctl* ic, uint64_t* data, int vector);

static
uint64_t apicIoEncode(struct apic_io_redir* red)
{
    return red->Vector 
            | (red->DeliveryMode << 8)
            | (red->DestinationMode << 11)
            | (red->DeliveryStatus << 12)
            | (red->PinPolarity << 13)
            | (red->RemoteIrr << 14)
            | (red->TriggerMode << 15)
            | (red->Mask << 16)
            | (red->Destination << 56);

}

static struct apic_io_redir 
apicIoDecode(uint64_t raw)
{
    struct apic_io_redir ret;
    ret.Vector = raw & 0xFF;
    ret.DeliveryMode = (raw >> 8) & 7;
    ret.DestinationMode = (raw >> 11) & 1;
    ret.DeliveryStatus  = (raw >> 12) & 1;
    ret.PinPolarity     = (raw >> 13) & 1;
    ret.RemoteIrr       = (raw >> 14) & 1;
    ret.TriggerMode     = (raw >> 15) & 1;
    ret.Mask            = (raw >> 14) & 1;
    ret.Destination     = (raw >> 56) & 255;
    return ret;
}

static struct apic_io_redir 
getRedirectionEntry(struct apic_int_ctl* this, int entry)
{
    uint16_t intr = 0x10 + entry * 2;
    uint32_t lower = IoValue(this->IoApicAddress, intr);
    uint32_t upper = IoValue(this->IoApicAddress, intr + 1);
    return apicIoDecode(((uint64_t)upper << 32) | lower);
}

static void
setRedirectionEntry(struct apic_int_ctl* this, int entry, struct apic_io_redir re)
{
    uint16_t intr = 0x10 + entry * 2;
    uint64_t encoded = apicIoEncode(&re);
    IoValue(this->IoApicAddress, intr) = encoded & 0xFFFFFFFF;
    IoValue(this->IoApicAddress, intr + 1) = encoded >> 32;
}

static void parseMadt(struct apic_int_ctl* restrict this, 
                      const struct acpi_madt* restrict madt)
{
    bool ovrlap = false;
    uint8_t* entries = (uint8_t*)PaAdd(madt, sizeof(struct acpi_madt));
    int remaining = madt->Header.Length - sizeof(struct acpi_madt);
    int entrySize = 0;
    do {
        switch (*entries) {
        case MADT_IO_APIC: {
            struct madt_entry_io_apic* addr = (struct madt_entry_io_apic*)(entries);
            this->IoApicAddress = (volatile uint32_t*)(uint64_t)(addr->IoApicAddress);
            this->GlobalSystemBase = addr->GsiBase;
            pgMapPage(NULL, (uint64_t)this->IoApicAddress, (uint64_t)this->IoApicAddress, 
                     PT_FLAG_PCD | PT_FLAG_WRITE);
            break;
        }
        case MADT_ENTRY_LOCAL_APIC_ADDR_OVERRIDE: {
            if (ovrlap) {
                trmLogfn("Local APIC address override specified more than once.\n");
                break;
            }

            struct madt_entry_local_apic_addr_override* addr = (typeof(addr))(entries);
            lapicAddress = (volatile uint32_t*)(addr->LocalApicAddress);
            ovrlap = true;
            break;
        }
        case MADT_IO_APIC_INT_SOURCE_OVERRIDE: {
            break;
        }
        default:
            break;
        }

        entrySize = *(entries + 1);
        remaining -= entrySize;
        entries += entrySize;
    } while (remaining > 0);
}

struct apic_int_ctl* apicCtlCreate()
{
    struct acpi_madt* madt = (struct acpi_madt*)acpiFindTable("APIC");
    if (!madt) {
        return NULL;
    }

    interruptHandlers = vectorCreate(3);

    struct apic_int_ctl* this = mmAllocKernelObject(struct apic_int_ctl);
    this->Base.isVectorFree = isVectorFree;
    this->Base.ackInterrupt = ackInterrupt;
    this->Base.getControllerType = getControllerType;
    this->handleInterruptIo = handleInterruptIo;
    this->Base.handleInterrupt = handleInterrupt;
    this->Base.executeInterrupt = executeInterrupt;
    this->Base.getMsiAddress = getMsiAddress;

    lapicAddress = (volatile uint32_t*)(uint64_t)madt->LocalApicAddress;
    pgAddGlobalPage((uint64_t)lapicAddress, (uint64_t)lapicAddress, PT_FLAG_WRITE | PT_FLAG_PCD);

    parseMadt(this, madt);

    lapicAddress[LAPIC_REG(0xF0)] = 47 | (1 << 8);
    return this;
}

/* Used by PIC. */
int isVectorFree(struct int_ctl* this, int vector)
{
    (void)this;
    (void)vector;
    return false;
}

int getControllerType(struct int_ctl* this)
{
    (void)this;
    return INT_CTL_TYPE_APIC;
}

void ackInterrupt(struct int_ctl* this)
{
    (void)this;
    lapicAddress[LAPIC_REG(0xB0)] = 0;
}

void handleInterruptIo(struct apic_int_ctl* this, int entry, int vector)
{
    struct apic_io_redir r = getRedirectionEntry(this, entry);
    r.Vector = vector;
    r.Destination = apicGetCurrentCpuId();
    r.Mask = 0;
    r.TriggerMode = 0;
    r.PinPolarity = 0;
    r.DestinationMode = 0;
    setRedirectionEntry(this, entry, r);
}

int apicGetCurrentCpuId()
{
    return lapicAddress[LAPIC_REG(0x20)] >> 24;
}

volatile uint32_t* apicGetLocalAddress() {
    return lapicAddress;
}

void handleInterrupt(struct int_ctl* ctl, int vector, int_handler_pfn_t ih, void *data)
{
    uint16_t v = idtGetFreeVector(2);
    struct int_h_data* h = mmAllocKernelObject(struct int_h_data);
    h->Data = data;
    h->Handler = ih;
    h->Vector = v;

    vectorInsert(&interruptHandlers, h);
    handleInterruptIo((struct apic_int_ctl*)ctl, vector, v);
}

void executeInterrupt(struct int_ctl* ctl, int vector)
{
    __unused(ctl);
    
    for (size_t i = 0; i < interruptHandlers.Length; i++) {
        struct int_h_data* h = interruptHandlers.Data[i];
        if (h->Vector == vector)    
            h->Handler(h->Data);
    }
}

static uint64_t getMsiAddress(struct int_ctl* ic, uint64_t* data, int vector)
{
    *data = (vector & 0xFF);
    return (0xFEE00000 | (apicGetCurrentCpuId() << 12));
}

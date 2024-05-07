#include "arch/i386/paging.h"
#include "driver.h"
#include "driver_kinterface.h"
#include "pci.h"
#include "ahci_state.h"
#include "term/terminal.h"

void driverMain(struct kdriver_manager*);

struct kdriver_manager* __driverManager = NULL;

static struct driver_conditional_loading cd[] = {
    {
        .RelationshipWithPrevious = DRIVER_CLD_RELATIONSHIP_NONE,
        .ConditionalType = DRIVER_CLD_TYPE_PCI,
        .HasNext = 0,
        .Pci = {
            .DeviceId = DRIVER_DONT_CARE,
            .VendorId = DRIVER_DONT_CARE,
            .Class    = 0x1,        /* Mass Storage controller */
            /* It is actually wrong to presume all
               SATA devices are AHCI! */
            .Subclass = 0x6         /* AHCI controller */
        }
    }
};

__attribute__((aligned(16)))
static struct driver_info info = {
    .Name = "ahci",
    .Role = DRIVER_ROLE_DISK,
    .ConditionalLoading = cd,
    .EntryPoint = driverMain,
    .Interface = NULL
};

struct driver_info* driverQuery()
{
    return &info;
}

void driverMain(struct kdriver_manager* mms)
{
    uint16_t cmd = pciReadDoubleWordFromDevice(mms->LoadReason, 0x4);
    cmd |= (1 << 8);
    pciWriteDoubleWordToDevice(mms->LoadReason, 0x4, cmd);
    info.Interface = ahciCreateState(mms->LoadReason);
    trmLogfn("exit from main");
}


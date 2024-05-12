#include "arch/i386/paging.h"
#include "display.h"
#include "driver_kinterface.h"
#include "mmio_reg.h"
#include "pci.h"
#include "state.h"
#include "term/terminal.h"
#include <driver.h>

static void driverMain(struct kdriver_manager* kdm);

static struct driver_conditional_loading cd[] = {
    {
        .RelationshipWithPrevious = DRIVER_CLD_RELATIONSHIP_NONE,
        .ConditionalType = DRIVER_CLD_TYPE_PCI,
        .HasNext = 0,
        .Pci = {
            .DeviceId = 0x0106,
            .VendorId = 0x8086,
            .Class    = 0x3,        /* Display controller */
            .Subclass = 0x0         /* VGA-compatible controller */
        }
    }
};


static struct driver_info info = {
    .Name = "i915",
    .Role = DRIVER_ROLE_DISPLAY,
    .ConditionalLoading = cd,
    .EntryPoint = driverMain,
    .Interface = NULL
};

struct driver_info* driverQuery()
{
    return &info;
}

void driverMain(struct kdriver_manager* kdm)
{
    trmLogfn("Hello, i915");
    struct pci_device* lr = kdm->LoadReason;
    struct i915* st = i915CreateState(lr);
    i915SetDisplayMode(st);
}

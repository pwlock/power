#include "atapi.h"
#include "driver.h"
#include "driver_kinterface.h"

#include "ide_state.h"
#include "pci.h"
#include "term/terminal.h"
#include <stddef.h>

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
            .Subclass = 0x1         /* IDE controller */
        }
    }
};

__attribute__((aligned(16)))
static struct driver_info info = {
    .Name = "ide",
    .Role = DRIVER_ROLE_DISK,
    .ConditionalLoading = cd,
    .EntryPoint = driverMain,
    .Interface = NULL
};

struct driver_info* driverQuery()
{
    return &info;
}

void driverMain(struct kdriver_manager* manager)
{
    __driverManager = manager;
    struct pci_device* device = manager->LoadReason;
    info.Interface = ideCreateState(device);
}
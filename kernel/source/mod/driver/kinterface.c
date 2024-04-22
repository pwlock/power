#include "config.h"
#include "driver_kinterface.h"
#include "memory/physical.h"
#include "mod/driver/driver.h"
#include "pci.h"

struct kdriver_manager* modCreateDriverManager(struct driver_info* info, 
                                               struct pci_device* device)
{
    __unused(info);
    struct kdriver_manager* man = mmAllocKernelObject(struct kdriver_manager);
    man->LoadReason = device;
    man->Info = info;
    return man;
}

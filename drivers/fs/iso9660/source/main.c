#include "config.h"
#include "driver.h"
#include "driver_kinterface.h"

#include "memory/physical.h"
#include "term/terminal.h"
#include "interface.h"

static void driverMain(struct kdriver_manager*);

static struct driver_info info = {
    .Name = "isofs",
    .Role = DRIVER_ROLE_FILESYSTEM,
    .ConditionalLoading = NULL,
    .EntryPoint = driverMain,
    .Interface = NULL
};

struct driver_info* driverQuery()
{
    return &info;
}

void driverMain(struct kdriver_manager* manager)
{
    __unused(manager);
    info.Interface = mmAllocKernelObject(struct isofs);
    registerVfsInterface(info.Interface);
}



#include "abstract/interrupt_ctl.h"
#include "acpi.h"
#include "arch/i386/ports.h"
#include "driver.h"
#include "driver_kinterface.h"
#include "intnotify.h"
#include "state.h"
#include "term/terminal.h"

static bool shouldLoadPs2();
void driverMain(struct kdriver_manager* man);

static struct driver_conditional_loading cd[] = {
    {
        .RelationshipWithPrevious = DRIVER_CLD_RELATIONSHIP_NONE,
        .ConditionalType = DRIVER_CLD_TYPE_CUSTOM_FUNC,
        .HasNext = 0,
        .shouldLoad = shouldLoadPs2,
    }
};

__attribute__((aligned(16)))
static struct driver_info info = {
    .Name = "ps2",
    .Role = DRIVER_ROLE_INPUT,
    .ConditionalLoading = cd,
    .EntryPoint = driverMain,
    .Interface = NULL
};

struct driver_info* driverQuery()
{
    return &info;
}

bool shouldLoadPs2()
{
    struct acpi_fadt* fadt = (struct acpi_fadt*)acpiFindTable("FACP");
    if (!fadt)
        return true;
    return fadt->BootArchitectureFlags & (1 << 1);
}

void driverMain(struct kdriver_manager* man)
{
    (void)man;
    struct ps2_state* p = ps2Init();
    p->KeyState = PS2_KEY_STATE_NORMAL;

    ps2SendDeviceCommandEx(p, false, 0xF0, 2);
    ps2SendDeviceCommandEx(p, false, 0xF3, 0b00111100);

    intCtlHandleInterrupt(1, kbdInterrupt, p);
    ps2EnableConfig(p, PS2_CONFIG_P1_INT);
}

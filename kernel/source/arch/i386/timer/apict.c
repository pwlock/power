#include "apict.h"
#include "abstract/interrupt_ctl.h"
#include "abstract/timer.h"
#include "arch/i386/int/apic.h"
#include "memory/physical.h"
#include "term/terminal.h"

static int awaitTimer(volatile uint32_t* lapic)
{
    struct timer* d = tmGetDefaultTimer();

    ApicValue(lapic, 0x3E0) = 0b0011; /* Divide by 16 */
    uint32_t v = ApicValue(lapic, 0x320);
    v &= ~(1 << 16);
    v |= 32 | (1 << 17);
    ApicValue(lapic, 0x320) = v;

    lapic[LAPIC_REG(0x380)] = 0xFFFFFFFF;
    tmSetReloadValue(d, 3000);

    uint32_t cnt = (0xFFFFFFF - lapic[LAPIC_REG(0x390)]) & 0xFFFF;
    lapic[LAPIC_REG(0x380)] = cnt;
    return cnt;
}

struct apic_timer* apicCreateTimer()
{
    if (intCtlGetControllerType() != INT_CTL_TYPE_APIC) {
        trmLogfn("apicCreateTimer failed: not using APIC");
        return NULL;
    }

    struct apic_timer* this = mmAlignedAlloc(sizeof(struct apic_timer), 1);
    volatile uint32_t* lp = apicGetLocalAddress();
    uint32_t ct = awaitTimer(lp);
    ApicValue(lp, 0x380) = ct;
    
    return this;
}

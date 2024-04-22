#include "hpet.h"
#include "abstract/interrupt_ctl.h"
#include "abstract/interrupt_ctl_interface.h"
#include "abstract/timer.h"
#include "acpi.h"
#include "arch/i386/int/apic.h"
#include "arch/i386/paging.h"
#include "arch/i386/timer/pit.h"
#include "memory/physical.h"
#include "term/terminal.h"

#define GENCAP_REG 0
#define GENCFG_REG 0x2
#define GENINT_REG 0x4
#define GENMCV_REG 0x1E

#define TMR_GEN_CALC(V, x) (V + (0x20 * x)) / sizeof(uint64_t)

#define TMRCFG_REG(N) TMR_GEN_CALC(0x100, N) /* Timer N Configuration and Capability Register */
#define TMRCMP_REG(N) TMR_GEN_CALC(0x108, N) /* Timer N Comparator Value Register */
#define TMRFSB_REG(N) TMR_GEN_CALC(0x110, N) /* Timer N FSB Interrupt Route Register */

volatile bool hpetSleepFlag = false;

static inline 
uint8_t findIrq(uint32_t irqs)
{
    uint8_t bitpos = 0;
    for (bitpos = 1; bitpos < 32; bitpos++) {
        if ((irqs & (1 << bitpos)) == 0)
            continue;

        return bitpos;
    }

    return -1;
}

static void setReloadValue(struct timer* this, uint64_t reload);
static uint64_t getMillisecondUnit(struct timer* this);
static int getType(struct timer* this);

struct hpet_timer*
hpetCreateTimer()
{
    struct acpi_system_desc_header* hph = acpiFindTable("HPET");
    if (!hph) {
        trmLogfn("hpetCreateTimer called... but no HPET table!");
        return NULL;
    }

    struct hpet_timer* timer = mmAlignedAlloc(sizeof(struct hpet_timer), 1);
    timer->Base.setReloadTime = setReloadValue;
    timer->Base.getType = getType;
    timer->Base.getMillisecondUnit = getMillisecondUnit;

    timer->TimerNumber = ((struct hpet_header*)hph)->TimerNumber;
    timer->BaseAddress = (uint64_t*)((struct hpet_header*)hph)->Address.Address;
    pgAddGlobalPage((uint64_t)timer->BaseAddress, 
                    (uint64_t)timer->BaseAddress, 
                    PT_FLAG_WRITE | PT_FLAG_PCD);
    {
        uint64_t gcfg = timer->BaseAddress[GENCAP_REG];
        timer->TimerPeriod = gcfg >> 32;
    }
    
    uint64_t cap = timer->BaseAddress[GENCAP_REG];
    { 
        /* We have to give higher priority to legacy intrs
           because Bochs simply maps PIC IRQS to 
           IOAPIC Redirection Entries. */
        if ((cap & (1 << 15))) {
            pitDisable();
            uint64_t tmrcfg = timer->BaseAddress[GENCFG_REG];
            tmrcfg |= 3; 
            timer->BaseAddress[GENCFG_REG] = tmrcfg;

            timer->ApicLine = 2;
        }
        else {
            uint32_t irqs = timer->BaseAddress[TMRCFG_REG(timer->TimerNumber)] >> 32;
            uint32_t iq = findIrq(irqs);

            timer->ApicLine = iq;
            timer->BaseAddress[GENCFG_REG] = 0b01;
        }

        if (intCtlGetControllerType() == INT_CTL_TYPE_APIC) {
            struct apic_int_ctl* ctl = (struct apic_int_ctl*)intCtlGetDefault();
            ctl->handleInterruptIo(ctl, timer->ApicLine, 33);
        }
    }

    timer->BaseAddress[TMRCFG_REG(timer->TimerNumber)] = (timer->ApicLine << 9) | (1 << 2);
    return timer;
}

void setReloadValue(struct timer* cls, uint64_t reload)
{
    struct hpet_timer* this = (struct hpet_timer*)cls;

    /* disable the timer */
    hpetSleepFlag = false;
    this->BaseAddress[TMRCMP_REG(this->TimerNumber)] = this->BaseAddress[GENMCV_REG] + reload;
    while (!hpetSleepFlag) {
        asm volatile("sti; hlt");
    }
}

uint64_t getMillisecondUnit(struct timer* cls)
{
    /* This assumes the HPET period is 100ns */
    struct hpet_timer* this = (struct hpet_timer*)cls;
    return this->TimerPeriod * 10000;
}

int getType(struct timer* cls)
{
    __unused(cls);
    return TIMER_TYPE_HPET;
}

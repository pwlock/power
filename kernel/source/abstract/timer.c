#include "timer.h"
#include "arch/i386/timer/hpet.h"

static struct timer* dflInstance;

void tmCreateTimer()
{
    void* ret;
    static timer_create_pfn_t creators[] = { (timer_create_pfn_t)hpetCreateTimer };
    for (size_t i = 0; i < ARRAY_LENGTH(creators); i++) {
        if ((ret = creators[i]()))
            break; 
    }

    dflInstance = ret;
    dflInstance->EnabledFlag = 1;
}

struct timer* tmGetDefaultTimer()
{
    return dflInstance;
}

void tmSetReloadValue(struct timer* this, size_t units)
{
    this->setReloadTime(dflInstance, units);
}

void tmDisable(struct timer* this)
{
    this->EnabledFlag = 0;
}

void tmEnable(struct timer* this)
{
    this->EnabledFlag = 1;
}

int tmGetType(struct timer* this)
{
    return this->getType(this);
}

int tmGetMillisecondUnit(struct timer* this)
{
    return this->getMillisecondUnit(this);
}

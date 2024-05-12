#include "panel.h"
#include "abstract/timer.h"
#include "mmio_reg.h"

void ipnDisableBacklight(struct i915* gpu)
{
    struct timer* tm = tmGetDefaultTimer();
    i915WriteBack(gpu, REG_BLC_PWM_CTL1, PCH_PWM_CTL1_ENABLE_BIT, 0);
    i915WriteBack(gpu, REG_PCH_PWM_CTL1, PCH_PWM_CTL1_ENABLE_BIT, 0);

    /* WORKAROUND because I do not know how can I be notified
       of when the backlight actually shuts down.  */
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 200);
    i915WriteBack(gpu, REG_PP_CTL, PP_CTL_POWER_STATE_BIT, 0);
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 100);

    i915WriteBack(gpu, REG_LVDS, LVDS_ENABLE_BIT, 0);
}

void ipnEnableLvds(struct i915* gpu)
{
    i915WriteBack(gpu, REG_LVDS, 0, LVDS_ENABLE_BIT);
}

void ipnEnableBacklight(struct i915* gpu, int level)
{
    struct timer* tm = tmGetDefaultTimer();
    i915WriteBack(gpu, REG_PP_CTL, 0, PP_CTL_POWER_STATE_BIT);
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 100);
    /*while (!(i915Read(gpu, REG_PP_STATUS) & PP_STATUS_ON_BIT)) {
        asm volatile("pause");
    }*/

    i915WriteBack(gpu, REG_BLC_PWM_CTL1, 0, PCH_PWM_CTL1_ENABLE_BIT);
    i915Write(gpu, REG_BLC_PWM_CTL2, level);
    i915WriteBack(gpu, REG_PCH_PWM_CTL1, 0, PCH_PWM_CTL1_ENABLE_BIT);
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 200);
}


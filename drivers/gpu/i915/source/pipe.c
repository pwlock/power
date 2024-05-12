#include "pipe.h"
#include "abstract/timer.h"
#include "config.h"
#include "memory/physical.h"
#include "mmio_reg.h"
#include "term/terminal.h"

struct i915_pipe* ippCreatePipe(struct i915* gpu, int index)
{
    struct i915_pipe* pp = mmAllocKernelObject(struct i915_pipe);
    pp->Index = index;
    pp->Device = gpu;
    pp->Enabled = (i915Read(gpu, REG_PIPE_CFG(index)) & PIPE_ENABLE_BIT) >> 31;

    return pp;
}

void ippDisablePipe(struct i915_pipe* pp)
{
    struct timer* tm = tmGetDefaultTimer();
    i915WriteBack(pp->Device, REG_PIPE_CFG(pp->Index), PIPE_ENABLE_BIT, 0);

    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 3);
    /*while (i915Read(pp->Device, REG_PIPE_CFG(pp->Index)) & PIPE_STATE_BIT) {
        asm volatile("pause");
    }*/

    pp->Enabled = false;
}

void ippEnablePipe(struct i915_pipe* pp)
{
    struct timer* tm = tmGetDefaultTimer();
    i915WriteBack(pp->Device, REG_PIPE_CFG(pp->Index), 0, PIPE_ENABLE_BIT);
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 3);
    /*while ((i915Read(pp->Device, REG_PIPE_CFG(pp->Index)) & PIPE_STATE_BIT) != 0) {
        asm volatile("pause");
    }*/

    pp->Enabled = true;
}

void ippSetTimings(struct i915_pipe* pp, const struct mode_description* md)
{
    if (pp->Enabled)
        return;

    uint32_t htotal = ((md->HorzSyncTotal - 1) << 16) | (md->HorzDisplay - 1);
    uint32_t hsync = ((md->HorzSyncEnd - 1) << 16) | (md->HorzSyncStart - 1);
    uint32_t hblank = ((md->HorzDisplay - 1) << 16) | (md->HorzSyncTotal - 1);

    uint32_t vtotal = ((md->VerticalSyncTotal - 1) << 16) | (md->VerticalDisplay - 1);
    uint32_t vsync = ((md->VerticalSyncEnd - 1) << 16) | (md->VerticalSyncStart - 1);
    uint32_t vblank = ((md->VerticalDisplay - 1) << 16) | (md->VerticalSyncTotal - 1);

    uint32_t pipesrc = ((md->HorzDisplay - 1) << 16) | (md->VerticalDisplay - 1);

    i915Write(pp->Device, REG_PIPE_HTOTAL(pp->Index), htotal);
    i915Write(pp->Device, REG_PIPE_HSYNC(pp->Index), hsync);
    i915Write(pp->Device, REG_PIPE_HBLANK(pp->Index), hblank);

    i915Write(pp->Device, REG_PIPE_VTOTAL(pp->Index), vtotal);
    i915Write(pp->Device, REG_PIPE_VSYNC(pp->Index), vsync);
    i915Write(pp->Device, REG_PIPE_VBLANK(pp->Index), vblank);
    i915Write(pp->Device, REG_PIPE_SRC(pp->Index), pipesrc);
}

void iptrcEnable(struct i915* gpu, i915_transcoder_t index)
{
    struct timer* tm = tmGetDefaultTimer();
    i915WriteBack(gpu, REG_TRANS_CONF(index), 0, TRANSCF_ENABLE_BIT);
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 3);
    /*while (!(i915Read(gpu, REG_TRANS_CONF(index)) & TRANSCF_STATE_BIT)) {
        __spinloop_hint();
    }*/
}

void iptrcDisable(struct i915* gpu, i915_transcoder_t index)
{
    struct timer* tm = tmGetDefaultTimer();
    i915WriteBack(gpu, REG_TRANS_CONF(index), TRANSCF_ENABLE_BIT, 0);
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 3);
    /*while (i915Read(gpu, REG_TRANS_CONF(index)) & TRANSCF_STATE_BIT) {
        __spinloop_hint();
    }*/
}

void iptrcSetTimings(struct i915* gpu, i915_transcoder_t index,
                     const struct mode_description* md)
{
    if (i915Read(gpu, REG_TRANS_CONF(index)) & TRANSCF_ENABLE_BIT) {
        return;
    }

    uint32_t htotal = ((md->HorzSyncTotal - 1) << 16) | (md->HorzDisplay - 1);
    uint32_t hsync = ((md->HorzSyncEnd - 1) << 16) | (md->HorzSyncStart - 1);
    uint32_t hblank = ((md->HorzDisplay - 1) << 16) | (md->HorzSyncTotal - 1);

    uint32_t vtotal = ((md->VerticalSyncTotal - 1) << 16) | (md->VerticalDisplay - 1);
    uint32_t vsync = ((md->VerticalSyncEnd - 1) << 16) | (md->VerticalSyncStart - 1);
    uint32_t vblank = ((md->VerticalDisplay - 1) << 16) | (md->VerticalSyncTotal - 1);

    i915Write(gpu, REG_TRANS_HTOTAL(index), htotal);
    i915Write(gpu, REG_TRANS_HSYNC(index), hsync);
    i915Write(gpu, REG_TRANS_HBLANK(index), hblank);

    i915Write(gpu, REG_TRANS_VTOTAL(index), vtotal);
    i915Write(gpu, REG_TRANS_VSYNC(index), vsync);
    i915Write(gpu, REG_TRANS_VBLANK(index), vblank);
}

void iptrcEnableDpll(struct i915* gpu, i915_transcoder_t trans)
{
    i915WriteBack(gpu, REG_DPLL_SEL, 0, TRANS_DPLL_ENABLE(trans));
}

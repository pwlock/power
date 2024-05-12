#include "display.h"

#include "abstract/timer.h"
#include "arch/i386/ports.h"
#include "bootloader_requests.h"
#include "dpll.h"
#include "memory/physical.h"
#include "mmio_reg.h"
#include "panel.h"
#include "pipe.h"
#include "power/video/edid.h"
#include "term/terminal.h"


/* DPLL related-code */

static void enable(struct i915* gpu, int vgaPipe, int bk, struct mode_description* md)
{
    struct i915_pipe* vgp = gpu->Pipes.Data[vgaPipe];
    struct timer* tm = tmGetDefaultTimer();
    int clear = 0, set = 0;
    if (vgaPipe != PIPE_A) 
        set = vgaPipe << 29;
    else
        clear = (unsigned)1 << 29;

    i915WriteBack(gpu, REG_LVDS, clear, set);

     /* ENABLE */
    /* Do everything from above in reverse order */
    i915WriteBack(gpu, REG_FDI_RX_CTL, 0, FDI_ENABLE_PLL_BIT);
    i915WriteBack(gpu, REG_FDI_TX_CTL, 0, FDI_ENABLE_PLL_BIT);

    /* PRM says 30us but we don't have a timer with usec precision. */
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 1);
    ipnEnableLvds(gpu);
    ipllSetDivisors(gpu, vgaPipe, md->PixelClock);
    iptrcSetTimings(gpu, vgaPipe, md);
    iptrcEnable(gpu, vgaPipe);

    ipllAssociateTranscoder(gpu, vgaPipe, vgaPipe);

    i915WriteBack(gpu, REG_DPLL_CTL(vgaPipe), 0, DPLL_VCO_ENABLE_BIT);

    /* A 150us halt would be great here. TODO. */
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 1);

    /* SET PIPE TIMINGS */
    ippSetTimings(vgp, md);
    ippEnablePipe(vgp);
    
    /* Update plane registers now so we don't
       have to wait for vblank  */
    i915WriteBack(gpu, REG_PLANE_CTL(vgaPipe), PLANE_TILING_BIT | PLANE_PIXF_BITS,
                  PLANE_PIXF_X8R8G8B8);
    i915Write(gpu, REG_PLANE_LINOFF(vgaPipe), 0);
    i915Write(gpu, REG_PLANE_SURFADDR(vgaPipe), 0);
    i915Write(gpu, REG_PLANE_STRIDE(vgaPipe), 2048);
    i915WriteBack(gpu, REG_PLANE_CTL(vgaPipe), 0, PLANE_ENABLE_BIT);

    ipnEnableBacklight(gpu, bk);
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 1000);
    trmLog("Hai!");
}

void i915SetDisplayMode(struct i915* gpu)
{
    struct limine_framebuffer_response* rfb = rqGetFramebufferRequest();
    struct limine_framebuffer* fb = rfb->framebuffers[0];
    struct edid* ed = fb->edid;
    struct edid_detailed_timing* dtm = &ed->Detailed[0];
    struct mode_description* md = i915CreateModeFromTiming(dtm);
    uint32_t bk = i915Read(gpu, REG_BLC_PWM_CTL2);
    uint32_t ob = i915Read(gpu, REG_PCH_PWM_CTL1);

    trmLogfn("bye, backlight level %i. Override? %i", bk,
             ob & PCH_PWM_CTL1_OVERRIDE_BIT);
    struct timer* tm = tmGetDefaultTimer();
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 2000);

    ipnDisableBacklight(gpu);
    outb(0x3C4, 0x01);
    outb(0x3C5, inb(0x3C5) | (1 << 5));
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 1);

    uint32_t vgaPipe = (i915WriteBack(gpu, REG_VGACTL, 0, VGACTL_DISP_DISABLE_BIT)
                        & VGACTL_DISP_PIPE_BIT)
        >> 29;

    struct i915_pipe* vgp = gpu->Pipes.Data[vgaPipe];
    ippDisablePipe(vgp);

    i915WriteBack(gpu, REG_PFIT_CTL1(0), PFIT_ENABLE_SCALER, 0);

    i915WriteBack(gpu, REG_DPLL_CTL(vgaPipe), DPLL_VCO_ENABLE_BIT, 0);

    i915WriteBack(gpu, REG_FDI_RX_CTL, FDI_ENABLE_PLL_BIT, 0);
    i915WriteBack(gpu, REG_FDI_TX_CTL, FDI_ENABLE_PLL_BIT, 0);
    iptrcDisable(gpu, vgaPipe);

    /* Switch back to rawclk */
    i915WriteBack(gpu, REG_FDI_RX_CTL, FDI_RX_PCDCLK_BIT, 0);
    
    tmSetReloadValue(tm, tmGetMillisecondUnit(tm) * 1);
    enable(gpu, vgaPipe, bk, md);
}

struct mode_description*
i915CreateModeFromTiming(const struct edid_detailed_timing* det)
{
    int vphys = det->VerticalImageSizeLower
        | ((det->HorizVertImageSizeUpper4 & EDID_DETAIL_VERTICAL_IMAGE_SIZE_BITS));
    int hphys = det->HorizImageSizeLower
        | ((det->HorizVertImageSizeUpper4 & EDID_DETAIL_HORIZONTAL_IMAGE_SIZE_BITS)
           >> EDID_DETAIL_HORIZONTAL_IMAGE_SIZE_SHIFT);
    trmLogfn("hphys=%i, vphys=%i", hphys, vphys);

    int hactive = det->HorizActivePixelLower
        | ((det->HorizBlankActiveUpper4 & EDID_DETAIL_ACTIVE_UPPER_BITS) << 4);
    int vactive = det->VerticalActivePixelLower
        | ((det->VerticalBlankActiveUpper4 & EDID_DETAIL_ACTIVE_UPPER_BITS) << 4);
    trmLogfn("hactive=%i, vactive=%i", hactive, vactive);

    int hsyncOffset = det->HorizFrontPorchLower
        | ((det->VerticalFrontPorchLower4 & EDID_DETAIL_HPORCH_UPPER_BITS) << 2);
    int vsyncOffset =
        ((det->VerticalFrontPorchLower4 & EDID_DETAIL_VPORCH_MIDDLE_BITS) >> 4)
        | ((det->HorizVertFrontPorch & EDID_DETAIL_VPORCH_UPPER_BITS) << 4);
    trmLogfn("hsyncOffset=%i, vsyncOffset=%i", hsyncOffset, vsyncOffset);

    int hsync = det->HorizSyncPulseLower
        | ((det->HorizVertFrontPorch & EDID_DETAIL_HSYNC_UPPER_BITS) << 4);
    int vsync = (det->VerticalFrontPorchLower4 & EDID_DETAIL_VSYNC_MIDDLE_BITS)
        | ((det->HorizVertFrontPorch & EDID_DETAIL_VSYNC_UPPER_BITS) << 4);
    trmLogfn("hsync=%i, vsync=%i", hsync, vsync);

    int hblank = det->HorizBlankPixelLower
        | ((det->HorizBlankActiveUpper4 & EDID_DETAIL_BLANK_UPPER_BITS) << 8);
    int vblank = det->VerticalBlankPixelLower
        | ((det->VerticalBlankActiveUpper4 & EDID_DETAIL_BLANK_UPPER_BITS) << 8);
    trmLogfn("hblank=%i, vblank=%i", hblank, vblank);

    struct mode_description* desc = mmAllocKernelObject(struct mode_description);
    desc->PixelClock = det->PixelClock * 10;
    desc->PhysicalHeight = vphys;
    desc->PhysicalWidth = hphys;

    desc->HorzDisplay = hactive;
    desc->HorzSyncStart = hactive + hsyncOffset;
    desc->HorzSyncEnd = hactive + hsync;
    desc->HorzSyncTotal = hactive + hblank;

    desc->VerticalDisplay = vactive;
    desc->VerticalSyncStart = vactive + vsyncOffset;
    desc->VerticalSyncEnd = vactive + vsync;
    desc->VerticalSyncTotal = vactive + vblank;

    return desc;
}
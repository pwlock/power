/* mmio_reg.h
   Purpose: MMIO GPU registers */
#pragma once

#include "config.h"
#include "memory/physical.h"
#include "state.h"
#include <stdint.h>

/* Interrupts */
#define REG_DISPLAY_IMR       0x44004
#define REG_DISPLAY_IER       0x4400C
#define REG_DISPLAY_IIR       0x44008
#define PCH_DPY_INTERRUPT_BIT (1 << 21)

/* VGA */
#define REG_VGACTL 0x41000
#define VGACTL_DISP_DISABLE_BIT (1 << 31)
#define VGACTL_DISP_PIPE_BIT   (1 << 29)

/* Pipe */
#define __PIPE(R, p) (R + (0x1000 * (p))) 

#define REG_PIPE_CFG(P) __PIPE(0x70008, P)
#define REG_PIPE_HTOTAL(P) __PIPE(0x60000, P)
#define REG_PIPE_HBLANK(P) __PIPE(0x60004, P)
#define REG_PIPE_HSYNC(P)  __PIPE(0x60008, P)
#define REG_PIPE_VTOTAL(P) __PIPE(0x6000C, P)
#define REG_PIPE_VBLANK(P) __PIPE(0x60010, P)
#define REG_PIPE_VSYNC(P) __PIPE(0x60014, P)
#define REG_PIPE_SRC(P)   __PIPE(0x6001C, P)
#define PIPE_ENABLE_BIT (1 << 31)
#define PIPE_STATE_BIT (1 << 30)

/* Plane */
#define REG_PLANE_CTL(P) __PIPE(0x70180, P)
#define PLANE_PIXF_SHIFT 26
#define PLANE_PIXF_BITS (0b1111 << PLANE_PIXF_SHIFT)
#define PLANE_PIXF_X8R8G8B8 (0b110 << PLANE_PIXF_SHIFT)

#define PLANE_TILING_BIT (1 << 10)
#define PLANE_ENABLE_BIT (1 << 31)

#define REG_PLANE_LINOFF(P) __PIPE(0x70184, P)
#define REG_PLANE_STRIDE(P) __PIPE(0x70188, P)
#define REG_PLANE_SURFADDR(P) __PIPE(0x7019C, P)

/* FDI */
/* Both for pipe A */
#define REG_FDI_RX_CTL 0xF000C
#define REG_FDI_TX_CTL 0x60100

#define FDI_ENABLE_PLL_BIT (1 << 13)
#define FDI_ENABLE_BIT (1 << 31)
#define FDI_RX_PCDCLK_BIT (1 << 4)

/* Panel fitter */
#define REG_PFIT_CTL1(P) 0x68080 + (0x800 * P)
#define PFIT_ENABLE_SCALER (1 << 31)

#define REG_BLC_PWM_CTL1    0x48250 /*< CPU */
#define REG_BLC_PWM_CTL2    0x48254 /*< CPU */

#define REG_PCH_PWM_CTL1     0xc8250
#define PCH_PWM_CTL1_ENABLE_BIT (1 << 31)
#define PCH_PWM_CTL1_OVERRIDE_BIT (1 << 30)
#define REG_PCH_PWM_CTL2     0xc8254

/* Panel power */
#define REG_PP_STATUS        0x61200
#define PP_STATUS_ON_BIT     (1 << 31)
#define REG_PP_CTL           0xC7204
#define PP_BACKLIGHT_BIT     (1 << 2)
#define PP_RESET_BIT         (1 << 1)
#define PP_CTL_POWER_STATE_BIT (1 << 0)

/* DPLL */
#define REG_DPLL_CTL(P)         (0x6014 + (4 * (P)))
#define REG_DPLL_FP0(P)         (0x6040 + (8 * (P)))
#define REG_DPLL_FP1(P)         (0x6044 + (8 * (P)))
#define REG_DPLL_SEL            0xc7000
#define TRANS_DPLLB_SEL(P)        ((1) << ((P) * 4))
#define TRANS_DPLL_ENABLE(P)      ((1) << ((P) * 4 + 3))

#define DPLL_VCO_ENABLE_BIT     (1 << 31)
#define DPLL_FP0_P1_POSTDIV(I)  ((1 << (I - 1)) << 16)
#define DPLL_FP1_P1_POSTDIV(I)  ((1 << (I - 1)))
#define DPLL_CLK_DIVIDE_BITS    (0b11 << 24)
#define DPLL_MODE_BITS          (0b11 << 26)
#define DPLL_LVDS_BIT           (0b10 << 26)

/* Transcoder */
#define REG_TRANS_CONF(P) __PIPE(0x70008, P)
#define TRANSCF_ENABLE_BIT (1 << 31)
#define TRANSCF_STATE_BIT (1 << 30)

#define REG_TRANS_HTOTAL(P) __PIPE(0x60000, P)
#define REG_TRANS_HBLANK(P) __PIPE(0x60004, P)
#define REG_TRANS_HSYNC(P) __PIPE(0x60008, P)
#define REG_TRANS_VTOTAL(P) __PIPE(0x6000C, P)
#define REG_TRANS_VBLANK(P) __PIPE(0x60010, P)
#define REG_TRANS_VSYNC(P) __PIPE(0x60014, P)

/* LVDS */
#define REG_LVDS 0xE1180
#define LVDS_ENABLE_BIT (1 << 31)
#define LVDS_SELTRANSB_BIT (1 << 30)

static inline uint32_t i915Read(struct i915* gpu, uint32_t reg)
{
    volatile uint32_t* r = PaAdd(gpu->MioAddress, reg);
    return *r;
}

static inline void i915Write(struct i915* gpu, uint32_t reg, uint32_t value)
{
    volatile uint32_t* r = PaAdd(gpu->MioAddress, reg);
    *r = value;
    __barrier();
}

static inline uint32_t i915WriteBack(struct i915* gpu, uint32_t reg, 
                                     uint32_t clearv, uint32_t setv)
{
    uint32_t val = i915Read(gpu, reg);
    val &= ~clearv;
    val |= setv;

    i915Write(gpu, reg, val);
    return val;
}

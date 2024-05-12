#include "dpll.h"
#include "display.h"
#include "mmio_reg.h"
#include "term/terminal.h"
#include "utils/abs.h"

struct i915_dotclk_limit {
    struct {
        int Min, Max;
    } Dot, Vco, N, M1, M2, P1;

    struct {
        int DotLimit;
        int Slow;
        int Fast;
    } P2;
};

/* for SNB */
static struct i915_dotclk_limit dotClock = {
  /* Dot.Max is LVDS' single-channel maximum clock frequency.
  Please change this to 350MHz if adding support for other
  non-LVDS connectors! */
    .Dot = { .Min = 25000, .Max = 112000 },
    .Vco = { .Min = 1760000, .Max = 3510000 },
    .N = { .Min = 3, .Max = 8 },
    .M1 = { .Min = 12, .Max = 22 },
    .M2 = { .Min = 5, .Max = 9 },
    .P1 = { .Min = 1, .Max = 8 },
    .P2 = { .DotLimit = 225000, .Slow = 14, .Fast = 14 }
};

static bool dpllValid(const struct dpll* dpll, const struct i915_dotclk_limit* limit);
static void calculateDpll(struct dpll* dpll, int refFreq);

/* Computes the DPLL frequency given requested pixel clock. */
static int getDpll(int pixelClock, int refFreq, struct dpll* dpll, struct dpll* best,
                   const struct i915_dotclk_limit* limit)
{
    int error = pixelClock;
    int localError;

    dpll->P2 = limit->P2.Slow; /* Fast is used for dual LVDS so ignore. */
    for (dpll->N = limit->N.Min; dpll->N <= limit->N.Max; dpll->N++) {
        for (dpll->M1 = limit->M1.Min; dpll->M1 <= limit->M1.Max; dpll->M1++) {
            for (dpll->M2 = limit->M2.Min; dpll->M2 <= limit->M2.Max; dpll->M2++) {
                for (dpll->P1 = limit->P1.Min; dpll->P1 <= limit->P1.Max;
                     dpll->P1++) {
                    calculateDpll(dpll, refFreq);
                    if (!dpllValid(dpll, limit))
                        continue;

                    localError = abs(dpll->Dot - pixelClock);
                    if (localError < error) {
                        (*best) = *dpll;
                        error = localError;
                    }
                }
            }
        }
    }

    return (error != pixelClock);
}

void calculateDpll(struct dpll* dpll, int refFreq)
{
    dpll->M = 5 * (dpll->M1 + 2) + (dpll->M2 + 2);
    dpll->P = dpll->P1 * dpll->P2;

    dpll->Vco = !(dpll->N + 2) ? 0 : (refFreq * dpll->M) / (dpll->N + 2);
    dpll->Dot = !dpll->P ? 0 : dpll->Vco / dpll->P;
}

bool dpllValid(const struct dpll* dpll, const struct i915_dotclk_limit* limit)
{
    return (dpll->Dot >= limit->Dot.Min && dpll->Dot <= limit->Dot.Max)
        && (dpll->Vco >= limit->Vco.Min && dpll->Vco <= limit->Vco.Max)
        && dpll->M1 > dpll->M2;
}

void ipllSetDivisors(struct i915* gpu, i915_dpll_t pll, int pxclk)
{
    struct dpll scratch, best;
    if (!getDpll(pxclk, 120000, &scratch, &best, &dotClock)) {
        trmLogfn("No suitable DPLL found");
        return;
    }

    uint32_t dptm = (best.N << 16) | (best.M1 << 8) | best.M2;
    i915WriteBack(gpu, REG_DPLL_CTL(pll), DPLL_CLK_DIVIDE_BITS | DPLL_MODE_BITS,
                  DPLL_FP0_P1_POSTDIV(best.P1) | DPLL_FP1_P1_POSTDIV(best.P1)
                      | DPLL_LVDS_BIT);
    i915WriteBack(gpu, REG_DPLL_FP0(pll), 0, dptm);
    i915WriteBack(gpu, REG_DPLL_FP1(pll), 0, dptm);
}

void ipllAssociateTranscoder(struct i915* gpu, i915_dpll_t pll,
                             i915_transcoder_t trans)
{
    int v = TRANS_DPLLB_SEL(trans);

    if (pll != PIPE_A)
        i915WriteBack(gpu, REG_DPLL_SEL, 0, v);
    else
        i915WriteBack(gpu, REG_DPLL_SEL, v, 0);
}

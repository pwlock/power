/* dpll.h
   Purpose: The display PLL */
#pragma once

#include "pipe.h"
#include "state.h"

typedef int i915_dpll_t;

void ipllSetDivisors(struct i915*, i915_dpll_t pll, int pxclk);
void ipllAssociateTranscoder(struct i915*, i915_dpll_t pll, i915_transcoder_t trans);

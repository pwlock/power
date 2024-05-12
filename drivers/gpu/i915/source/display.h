/* display.h
   Purpose: */
#pragma once

#include "power/video/edid.h"
#include "state.h"

struct dpll {
    int M1, M2, P1, P2, N;

    int Vco, Dot, M, P;
};

struct mode_description {
    int PixelClock; /*< in kHz*/
    uint16_t HorzDisplay; /*< Visible horiz display area in px. */
    uint16_t HorzSyncStart;
    uint16_t HorzSyncEnd;
    uint16_t HorzSyncTotal;

    uint16_t VerticalDisplay;
    uint16_t VerticalSyncStart;
    uint16_t VerticalSyncEnd;
    uint16_t VerticalSyncTotal;

    uint16_t PhysicalHeight; /*< In mm */
    uint16_t PhysicalWidth; /*< In mm */
};

struct mode_description*
i915CreateModeFromTiming(const struct edid_detailed_timing* det);
void i915SetDisplayMode(struct i915*);

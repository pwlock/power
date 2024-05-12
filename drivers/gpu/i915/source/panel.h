/* panel.h
   Purpose: */
#pragma once

#include "state.h"

void ipnEnableLvds(struct i915*);
void ipnEnableBacklight(struct i915*, int level);
void ipnDisableBacklight(struct i915*);

/* pipe.h
   Purpose: */
#pragma once

#include "display.h"
#define PIPE_A 0
#define PIPE_B 1

struct i915_pipe {
    struct i915* Device;
    int Index;
    int Enabled;
};

struct i915_pipe* ippCreatePipe(struct i915*, int index);
void ippSetTimings(struct i915_pipe*, const struct mode_description* md);
void ippEnablePipe(struct i915_pipe*);
void ippDisablePipe(struct i915_pipe*);

typedef int i915_transcoder_t;

/* Transcoders are so functionally similar to pipes
   that I think it makes sense to put this here. */
void iptrcEnable(struct i915*, i915_transcoder_t index);
void iptrcDisable(struct i915*, i915_transcoder_t index);
void iptrcSetTimings(struct i915*, i915_transcoder_t index,
                     const struct mode_description* md);
void iptrcEnableDpll(struct i915*, i915_transcoder_t t);

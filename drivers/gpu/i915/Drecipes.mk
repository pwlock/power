I915_BUILD=build/drivers/i915
I915_SOURCE=drivers/gpu/i915/source
I915_OUTPUT=$(I915_BUILD)/i915.kd

I915_CFILES=main.c state.c display.c panel.c pipe.c dpll.c
I915_CSOURCES:=$(addprefix $(I915_SOURCE)/,$(I915_CFILES))
I915_CBUILD:=$(addprefix $(I915_BUILD)/,$(addsuffix .o,$(I915_CFILES)))

AUXFS_FILES+=$(I915_OUTPUT)

$(I915_OUTPUT): $(I915_CBUILD) $(KERNEL_STUB)
	mkdir -p $(I915_BUILD)
	$(CC) $(DRIVER_LDFLAGS) $(I915_CBUILD) -o $(I915_OUTPUT)

$(I915_BUILD)/%.c.o: $(I915_SOURCE)/%.c
	mkdir -p $(I915_BUILD)/$(dir $(subst $(I915_SOURCE)/,,$<))
	$(CC) $(CFLAGS) $(DRIVER_CFLAGS) -c $< -o $@

i915_clean:
	rm -rf $(I915_BUILD)
	mkdir -p $(I915_BUILD)

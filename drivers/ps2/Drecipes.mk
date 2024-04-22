PS2_BUILD=build/drivers/ps2
PS2_SOURCE=drivers/ps2/source
PS2_OUTPUT=$(PS2_BUILD)/ps2.kd

PS2_CFILES=main.c state.c intnotify.c
PS2_CSOURCES:=$(addprefix $(PS2_SOURCE)/,$(PS2_CFILES))
PS2_CBUILD:=$(addprefix $(PS2_BUILD)/,$(addsuffix .o,$(PS2_CFILES)))

AUXFS_FILES+=$(PS2_OUTPUT)

$(PS2_OUTPUT): $(PS2_CBUILD) $(KERNEL_STUB)
	mkdir -p $(PS2_BUILD)/$(dir $(subst $(PS2_SOURCE)/,,$<))
	$(CC) $(DRIVER_LDFLAGS) $(PS2_CBUILD) -o $@

$(PS2_BUILD)/%.c.o: $(PS2_SOURCE)/%.c
	mkdir -p $(PS2_BUILD)/$(dir $(subst $(PS2_SOURCE)/,,$<))
	$(CC) $(CFLAGS) $(DRIVER_CFLAGS) -c $< -o $@


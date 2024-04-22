IDE_BUILD=build/drivers/ide
IDE_SOURCE=drivers/ide/source
IDE_OUTPUT=$(IDE_BUILD)/ide.kd

IDE_CFILES=main.c ide_state.c atapi.c ata.c ide_interface.c
IDE_CSOURCES:=$(addprefix $(IDE_SOURCE)/,$(IDE_CFILES))
IDE_CBUILD:=$(addprefix $(IDE_BUILD)/,$(addsuffix .o,$(IDE_CFILES)))

AUXFS_FILES+=$(IDE_OUTPUT)

$(IDE_OUTPUT): $(IDE_CBUILD) $(KERNEL_STUB)
	$(CC) $(DRIVER_LDFLAGS) $(IDE_CBUILD) -o $@

$(IDE_BUILD)/%.d: $(IDE_SOURCE)/%.c
	mkdir -p build/$(dir $(subst source/,,$^))	
	$(CC) $(CFLAGS) $(DRIVER_CFLAGS) -MMD -MT '$(patsubst $(IDE_SOURCE)/%.c,$(IDE_BUILD)/%.c.o,$<)' $< -MF $@ > /dev/null
	rm $(patsubst %.d,%.o,$(notdir $@))

$(IDE_BUILD)/%.c.o: $(IDE_SOURCE)/%.c $(IDE_BUILD)/%.d
	mkdir -p $(IDE_BUILD)/$(dir $(subst $(IDE_SOURCE)/,,$<))
	$(CC) $(CFLAGS) $(DRIVER_CFLAGS) -c $< -o $@

ide: $(IDE_OUTPUT) iso/$(ISO_OUTPUT)

ide_clean:
	rm -rf $(IDE_BUILD)
	mkdir -p $(IDE_BUILD)

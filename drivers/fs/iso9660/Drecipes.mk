ISOFS_BUILD=build/drivers/fs/iso
ISOFS_SOURCE=drivers/fs/iso9660/source
ISOFS_OUTPUT=$(ISOFS_BUILD)/isofs.kd

ISOFS_CFILES=main.c interface.c mount.c susp.c
ISOFS_CSOURCES:=$(addprefix $(ISOFS_SOURCE)/,$(ISOFS_CFILES))
ISOFS_CBUILD:=$(addprefix $(ISOFS_BUILD)/,$(addsuffix .o,$(ISOFS_CFILES)))

AUXFS_FILES+=$(ISOFS_OUTPUT)

$(ISOFS_OUTPUT): $(ISOFS_CBUILD) $(KERNEL_STUB)
	$(CC) $(LDFLAGS) $(DRIVER_LDFLAGS) $(ISOFS_CBUILD) -o $@

$(ISOFS_BUILD)/%.d: $(ISOFS_SOURCE)/%.c
	mkdir -p $(ISOFS_BUILD)/$(dir $(subst $(ISOFS_SOURCE)/,,$<))
	$(CC) $(CFLAGS) $(DRIVER_CFLAGS) -MMD -MT '$(patsubst $(ISOFS_SOURCE)/%.c,$(ISOFS_BUILD)/%.c.o,$<)' $< -MF $@ > /dev/null
	rm $(patsubst %.d,%.o,$(notdir $@))

$(ISOFS_BUILD)/%.c.o: $(ISOFS_SOURCE)/%.c $(ISOFS_BUILD)/%.d
	mkdir -p $(ISOFS_BUILD)/$(dir $(subst $(ISOFS_SOURCE)/,,$<))
	$(CC) $(CFLAGS) $(DRIVER_CFLAGS) -c $< -o $@

isofs: $(ISOFS_OUTPUT) iso/$(ISO_OUTPUT)

isofs_clean:
	rm -rf $(ISOFS_BUILD)
	mkdir -p $(ISOFS_BUILD)

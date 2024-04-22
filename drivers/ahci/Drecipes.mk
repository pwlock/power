AHCI_BUILD=build/drivers/ahci
AHCI_SOURCE=drivers/ahci/source
AHCI_OUTPUT=$(AHCI_BUILD)/ahci.kd

AHCI_CFILES=main.c ahci_state.c ahci_device.c scsi.c ahci_read.c ahci_interface.c
AHCI_CSOURCES:=$(addprefix $(AHCI_SOURCE)/,$(AHCI_CFILES))
AHCI_CBUILD:=$(addprefix $(AHCI_BUILD)/,$(addsuffix .o,$(AHCI_CFILES)))

AUXFS_FILES+=$(AHCI_OUTPUT)

$(AHCI_OUTPUT): $(AHCI_CBUILD) $(KERNEL_STUB)
	mkdir -p $(AHCI_BUILD)
	$(CC) $(DRIVER_LDFLAGS) $(AHCI_CBUILD) -o $(AHCI_OUTPUT)

$(AHCI_BUILD)/%.c.o: $(AHCI_SOURCE)/%.c
	mkdir -p $(AHCI_BUILD)/$(dir $(subst $(AHCI_SOURCE)/,,$<))
	$(CC) $(CFLAGS) $(DRIVER_CFLAGS) -c $< -o $@

ahci_clean:
	rm -rf $(AHCI_BUILD)
	mkdir -p $(AHCI_BUILD)

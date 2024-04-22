ENABLED_DRIVERS=ide fs/iso9660 ps2 ahci

DRIVER_LDFLAGS=-ffreestanding -nostdlib -Wl,-e -Wl,driverQuery -L$(KERNEL_BUILD) -l__r-kernelapi \
				-fpic -finline-functions
DRIVER_CFLAGS=-Ikernel/source -Ikernel/include -Ikernel/source/mod/driver -fpic

$(foreach m,$(ENABLED_DRIVERS),$(eval include drivers/$(m)/Drecipes.mk))

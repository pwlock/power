ISO_OUTPUT=power.iso
SYSTEM_ROOT=$$SYSROOT

AS=nasm
CC=clang
LD=lld.ld

ifeq ($(CC),clang)
CC+=-target x86_64-pc-unknown-elf
endif

ASFLAGS=-g -F dwarf -f elf64
CFLAGS=-g -mgeneral-regs-only -c -ffreestanding -fno-stack-protector -nostdlib \
	-fno-stack-check -fno-lto -fno-stack-protector -m64 \
	-march=x86-64 -mabi=sysv -mno-80387 -mno-red-zone -Wno-unused-command-line-argument \
	-Wno-address-of-packed-member -Wall -Wextra -std=gnu17 -fvisibility=default
LDFLAGS=-nostdlib -Wl,-m -Wl,elf_x86_64 -Wl,-z -Wl,max-page-size=0x1000

all: iso/$(ISO_OUTPUT)

clean:
	rm -rf build/
	mkdir build

MODULES=kernel drivers auxfs
$(foreach m,$(MODULES),$(eval include $(m)/recipes.mk))

iso/root:
	mkdir -p iso/root/EFI/BOOT

iso/$(ISO_OUTPUT): iso/root $(KERNEL_OUTPUT) $(AUXFS_PATH) $(TEST_OUTPUT) FORCE
	cp -v $(KERNEL) limine/limine-bios.sys    \
		limine/limine-bios-cd.bin limine.cfg  \
		limine/limine-uefi-cd.bin		      \
		$(KERNEL_OUTPUT)					  \
		$(AUXFS_PATH)						  \
		$(TEST_OUTPUT)						  \
		iso/root/
	cp -v limine/BOOTX64.EFI iso/root/EFI/BOOT/
	xorriso -as mkisofs -b limine-bios-cd.bin \
			-no-emul-boot -boot-load-size 4 -boot-info-table \
			 --protective-msdos-label \
			 --efi-boot limine-uefi-cd.bin \
			 -efi-boot-part --efi-boot-image \
			iso/root/ -o iso/$(ISO_OUTPUT)
	./limine/limine bios-install iso/$(ISO_OUTPUT)

qemu: iso/$(ISO_OUTPUT)
	~/pkg/qemu-install/bin/qemu-system-x86_64                                 \
		-accel tcg,thread=single                       \
		-m 128                                         \
		-no-reboot                                     \
		-drive format=raw,media=cdrom,file=iso/power.iso,index=0 \
		-drive media=disk,file=loc.img				   \
		-serial stdio                                  \
		-smp 1                                         \
		-usb                                           \
		-vga std -d guest_errors -no-reboot -no-shutdown -S -s \
		-cpu Haswell								   \
		-trace "pci_cfg_write"						   \
		-trace "*pci*"									\
		-trace "ahci*" -trace "ide*" \
		-boot d -M q35

FORCE:


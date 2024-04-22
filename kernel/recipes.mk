KERNEL_OUTPUT=build/kernel/oskrnl.elf

KERNEL_BUILD=build/kernel

-include kernel/source/arch/i386/sources.mk

KERNEL_ABSTRACT_CFILES=abstract/interrupt_ctl.c abstract/timer.c
KERNEL_MEMORY_CFILES=memory/physical.c memory/address.c memory/user_heap.c memory/ring.c
KERNEL_MOD_CFILES=mod/ustar.c mod/elf.c mod/driver/loader.c mod/driver/kinterface.c \
				  mod/ld/elf_ld.c mod/elf_sym.c mod/driver/autoloader.c mod/ld/elf_hashsym.c
KERNEL_SCHEDULER_CFILES=scheduler/scheduler.c scheduler/thread.c scheduler/synchronization.c
KERNEL_TERM_CFILES=term/toshibatxl1.c term/terminal.c
KERNEL_UM_CFILES=um/process.c um/syscall.c um/ioctl.c um/input.c
KERNEL_UTILS_CFILES=utils/string.c utils/vector.c utils/cmd.c

KERNEL_CFILES=main.c bootloader_requests.c acpi.c partition.c pci.c vfs.c \
			$(KERNEL_TERM_CFILES) $(KERNEL_UTILS_CFILES) \
			$(KERNEL_MEMORY_CFILES) $(KERNEL_ARCH_CFILES) $(KERNEL_ABSTRACT_CFILES) \
			$(KERNEL_SCHEDULER_CFILES) $(KERNEL_UM_CFILES) $(KERNEL_MOD_CFILES)

KERNEL_ASFILES=$(KERNEL_ARCH_ASFILES)

KERNEL_ASSOURCES:=$(addprefix $(KERNEL_BUILD)/,$(addsuffix .o,$(KERNEL_ASFILES)))
KERNEL_CSOURCES:=$(addprefix $(KERNEL_BUILD)/,$(addsuffix .o,$(KERNEL_CSOURCES)))
KERNEL_FILES=$(KERNEL_CFILES) $(KERNEL_ASFILES)
KERNEL_SOURCES=$(addprefix $(KERNEL_BUILD)/,$(addsuffix .o,$(KERNEL_FILES)))
KERNEL_OBJS:=$(addprefix $(KERNEL_BUILD)/,$(addsuffix .o,$(KERNEL_FILES)))

KERNEL_CFLAGS=$(CFLAGS) -Ilimine -Ikernel/source -mcmodel=kernel -fno-pic -fno-pie -I$(KERNEL_INCLUDE_DIR)
KERNEL_LDFLAGS=-nostdlib -static -Wl,-m -Wl,elf_x86_64 -Wl,-z -Wl,max-page-size=0x1000 \
			   -Wl,-T -Wl,kernel/linker.ld

# =============================== HEADERS
KERNEL_INCLUDE_DIR=kernel/include
KERNEL_HEADERS=$(KERNEL_INCLUDE_DIR)/power/system.h $(KERNEL_INCLUDE_DIR)/power/ioctl.h \
				$(KERNEL_INCLUDE_DIR)/power/input.h $(KERNEL_INCLUDE_DIR)/power/input_code.h 
KERNEL_INSTALLED_HEADERS=$(addprefix $(SYSTEM_ROOT)/System/Headers/,$(subst $(KERNEL_INCLUDE_DIR)/,,$(KERNEL_HEADERS)))
# ================================

KERNEL_STUB=$(KERNEL_BUILD)/lib__r-kernelapi.so

$(KERNEL_STUB): $(KERNEL_OUTPUT)
	python tools/stub.py $(KERNEL_OUTPUT) $(KERNEL_BUILD)/

$(KERNEL_OUTPUT): $(KERNEL_OBJS) kernel/linker.ld
	$(CC) $(LDFLAGS) $(KERNEL_LDFLAGS) $(KERNEL_OBJS) -o $@

build/kernel/%.d: kernel/source/%.c
	mkdir -p build/$(dir $(subst source/,,$^))	
	$(CC) $(KERNEL_CFLAGS) -MMD -MT '$(patsubst kernel/source/%.cpp,build/kernel/%.c.o,$<)' $< -MF $@ > /dev/null
	rm $(patsubst %.d,%.o,$(notdir $@))

build/kernel/%.c.o: kernel/source/%.c build/kernel/%.d
	mkdir -p build/kernel/$(dir $(subst kernel/source/,,$<))
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

build/kernel/%.s.o: kernel/source/%.s
	mkdir -p build/$(dir $(subst source/,,$^))
	$(AS) $(ASFLAGS) $^ -o $@

$(SYSTEM_ROOT)/System/Headers/power/%.h: $(KERNEL_INCLUDE_DIR)/power/%.h
	install -C -D -t $(SYSTEM_ROOT)/System/Headers/$(dir $(subst $(KERNEL_INCLUDE_DIR)/,,$^)) $^ -v

kernel_install: $(KERNEL_INSTALLED_HEADERS)

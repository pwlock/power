#include "abstract/interrupt_ctl.h"
#include "abstract/timer.h"

#include "arch/i386/gdt.h"
#include "arch/i386/idt.h"
#include "arch/i386/paging.h"

#include "bootloader_requests.h"
#include "config.h"
#include "memory/physical.h"
#include "mod/driver/autoloader.h"
#include "mod/driver/driver.h"
#include "mod/driver/driver_kinterface.h"
#include "mod/driver/loader.h"
#include "partition.h"
#include "pci.h"
#include "scheduler/scheduler.h"
#include "um/input.h"
#include "um/process.h"

#include "limine.h"
#include "scheduler/thread.h"
#include "term/terminal.h"
#include "utils/cmd.h"
#include "vfs.h"

#include "utils/string.h"
#include <stddef.h>

extern void cpuEnableFeatures();

static inline struct limine_memmap_entry* getUsableEntry()
{
    struct limine_memmap_entry* c;
    struct limine_memmap_entry* ret = NULL;
    struct limine_memmap_response* re = rqGetMemoryMapRequest();
    uint64_t max = 0;

    for (uint64_t i = 0; i < re->entry_count; i++) {
        c = re->entries[i];
        if (c->type == LIMINE_MEMMAP_USABLE
            && c->length > max) {
            max = c->length;
            ret = c;
        }
    }

    return ret;
}

static void secondInit(void*);

void keMain()
{
    trmInit();
    gdtInit();
    idtInit();
    cpuEnableFeatures();

    struct limine_memmap_response* re = rqGetMemoryMapRequest();
    struct limine_memmap_entry* ent = getUsableEntry();
    mmInit((void*)ent->base, ent->length);

    struct limine_memmap_entry* c;
    for (uint64_t i = 0; i < re->entry_count; i++) {
        c = re->entries[i];
        if (c->type == LIMINE_MEMMAP_USABLE
            && c->base != ent->base) {
            mmAddUsablePart((void*)c->base, c->length);
        }
    }

    pgSetCurrentAddressSpace(pgCreateAddressSpace());
    intCtlCreateDefault();
    tmCreateTimer();
    tmCreateSecondaryTimer();

    schedCreate();
    schedAddThread(schedCreateThread(secondInit, 0));
    schedEnable(true);

    for (;;)
        asm volatile("hlt");
}

void secondInit(void* nothing)
{
    __unused(nothing);
    pciInitializeRegistry();

    asm volatile("sti");

    modAutomaticLoad();
    struct kdriver_manager* man = modGetDriver("ahci.kd");
    struct driver_disk_interface* di = man->Info->Interface;
    struct driver_disk_device_interface* ddi = di->getDevice(di, 0);
    uint8_t buff;
    ddi->readSector(ddi, 0, 1, &buff);
    partInit(di);

    vfsInit();
    inpCreateInputRing();
    const char* fsType;
    cmdGetCommandArgument("fstype", &fsType);
    if (!strncmp("iso9660", fsType, 7)) {
        /* ISO9660 is not used in a partitioned space. */
        vfsCreateFilesystem("iso9660", 0, ddi);
    }

    pcCreateProcess(NULL, "A:/System/linit.elf", NULL);
}

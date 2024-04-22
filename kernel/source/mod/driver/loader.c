#include "loader.h"

#include "arch/i386/paging.h"
#include "bootloader_requests.h"
#include "driver.h"
#include "memory/physical.h"

#include "mod/driver/driver_kinterface.h"
#include "mod/driver/kinterface.h"
#include "mod/elf.h"
#include "mod/ld/elf_ld.h"
#include "mod/ustar.h"

#include "pci.h"
#include "term/terminal.h"
#include "utils/vector.h"
#include "utils/string.h"

struct vector loadedDrivers;

static struct pci_device* hasDevicePci(struct driver_cld_pci* conds)
{
    struct pci_search_query query;
    query.Class = conds->Class;
    query.SubClass = conds->Subclass;
    query.DeviceId = conds->DeviceId;
    query.Vendor = conds->VendorId;

    return pciSearchDeviceEx(query);
}

static 
struct pci_device* shouldLoadDriver(struct driver_conditional_loading* cld, 
                                    struct elf_executable* exe) 
{
    if (!cld) 
        return (void*)0x1; /* Only an issue if the driver 
                             actually tries do deref that */

    int index = 0;
    bool lastiHad = false;
    struct driver_conditional_loading i;
    struct pci_device* dev = NULL;

    do {
        i = cld[index];
        if (i.RelationshipWithPrevious == DRIVER_CLD_RELATIONSHIP_AND
         && !lastiHad) {
            return NULL;
        }
        
        switch(i.ConditionalType) {
        case DRIVER_CLD_TYPE_PCI:
            lastiHad = (dev = hasDevicePci(&i.Pci)) != NULL;
            break;
        case DRIVER_CLD_TYPE_CUSTOM_FUNC:
            lastiHad = ((driver_should_load_pfn_t)PaAdd(i.shouldLoad, exe->Base))();
            dev = (void*)0x1;
            break;
        }
        index++;
    } while(i.HasNext);

    return dev;
}

struct kdriver_manager*
modLoadDriver(const char* modName)
{
    return modLoadDriverEx(modName, true);
}

struct kdriver_manager* 
modLoadDriverEx(const char* modName, bool insideAuxfs)
{
    static bool createdVector = false;
    if (!createdVector) {
        loadedDrivers = vectorCreate(3);
        createdVector = true;
    }

    if (!insideAuxfs) {
        trmLogfn("Loading modules outside auxfs is"  
                 "currently not supported. Sorry!");
        return NULL;
    }

    const uint8_t* buffer;
    struct limine_module_response* m = rqGetModuleResponse();
    struct limine_file* f = m->modules[0];

    modLoadUstarFile(f->address, modName, &buffer);
    struct elf_executable exec = modLoadElf(buffer, ELF_LOAD_DRIVER);
    struct driver_info* info = ((driver_query_pfn_t)(exec.EntryPoint))();

    struct pci_device* dev;
    void* cld = NULL;
    if (info->ConditionalLoading)
        cld = PaAdd(info->ConditionalLoading, exec.Base);

    if (!(dev = shouldLoadDriver(cld, &exec))) {
        trmLogfn("(for driver %s) Conditional loading failed", PaAdd(info->Name, exec.Base));
        return NULL;
    }

    struct kdriver_manager* man = modCreateDriverManager(info, dev);
    ((driver_main_pfn_t)PaAdd(info->EntryPoint, exec.Base))(man);
    man->Info->Name = PaAdd(man->Info->Name, exec.Base);

    struct driver_internal_data* id = mmAllocKernelObject(struct driver_internal_data);
    id->Driver = man;
    id->Pages = exec.LoadedPages;
    vectorInsert(&loadedDrivers, id);

    return man;
}

void modMapDrivers(address_space_t* address)
{
    for (size_t i = 0; i < loadedDrivers.Length; i++) {
        struct driver_internal_data* id = loadedDrivers.Data[i];
        for (size_t ix = 0; ix < id->Pages.Length; ix++) {
            struct loaded_page* lp = id->Pages.Data[ix];
            pgMapPage(address, lp->PhysicalAddress,
                      lp->VirtualAddress, lp->Flags);
        }
    }
}

struct kdriver_manager* modGetDriver(const char* modName)
{
    for (size_t i = 0; i < loadedDrivers.Length; i++) {
        struct driver_internal_data* id = loadedDrivers.Data[i];
        if (!strcmp(modName, id->Driver->Info->Name)) {
            return id->Driver;
        }
    }

    return NULL;
}

struct kdriver_manager* modGetFilesystemName(const char* fsName)
{
    for (size_t i = 0; i < loadedDrivers.Length; i++) {
        struct driver_internal_data* id = loadedDrivers.Data[i];
        if (id->Driver->Info->Role != DRIVER_ROLE_FILESYSTEM)
            continue;
        
        struct driver_fs_interface* in = id->Driver->Info->Interface;
        if (!strcmp(fsName, in->Name)) {
            return id->Driver;
        }
    }

    return NULL;
}

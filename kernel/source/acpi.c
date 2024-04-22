#include "acpi.h"
#include "bootloader_requests.h"
#include "limine.h"
#include "memory/physical.h"
#include "term/terminal.h"
#include "utils/string.h"

#include <stddef.h>

static struct acpi_system_desc_header*
findTableXsdt(const struct acpi_system_desc_header* root, const char* restrict name)
{
    uint64_t hhdm = rqGetHhdmRequest()->offset;
    const uint64_t* ptrarray = PaAdd(root, sizeof(*root));
    int arraylength = (root->Length - sizeof(*root)) / sizeof(uint64_t);

    struct acpi_system_desc_header* ptr;
    for (int i = 0; i < arraylength; i++) {
        ptr = (typeof(ptr))(ptrarray[i] + hhdm);
        if (!strncmp(name, ptr->Signature, 4)) {
            return ptr;   
        }
    }
    return NULL;
}

struct acpi_system_desc_header* acpiGetRootTable(bool* wasRsdt)
{
    uint64_t hhdm = rqGetHhdmRequest()->offset;
    struct limine_rsdp_response* rsdp = rqGetRsdpResponse();
    struct acpi_rsdp* rootd = rsdp->address;

    uint64_t address;
    if (rootd->Revision >= 2) {
        address = rootd->XsdtAddress + hhdm;
        if (wasRsdt) 
            (*wasRsdt) = false;
    } else {
        address = rootd->RsdtAddress + hhdm;
        if (wasRsdt) 
            (*wasRsdt) = true;
    }

    return (struct acpi_system_desc_header*)address;
}

struct acpi_system_desc_header* 
acpiFindTable(const char* restrict name)
{
    bool rsdt;
    struct acpi_system_desc_header* root = acpiGetRootTable(&rsdt);

    if (!rsdt)
        return findTableXsdt(root, name);

    uint64_t hhdm = rqGetHhdmRequest()->offset;
    const uint32_t* ptrarray = PaAdd(root, sizeof(*root));
    int arraylength = (root->Length - sizeof(*root)) / sizeof(uint32_t);

    struct acpi_system_desc_header* ptr;
    for (int i = 0; i < arraylength; i++) {
        ptr = (typeof(ptr))(ptrarray[i] + hhdm);
        if (!strncmp(name, ptr->Signature, 4)) {
            return ptr;   
        }
    }

    return NULL;
}

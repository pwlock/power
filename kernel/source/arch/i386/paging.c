#include "paging.h"
#include "bootloader_requests.h"
#include "config.h"
#include "memory/address.h"
#include "memory/physical.h"
#include "term/terminal.h"
#include "utils/vector.h"

#include <power/system.h>

#define PAGE_CHECK(P, i) PAGE_CHECK_RET_VAL(P, i, )
#define PAGE_CHECK_RET_VAL(P, i, r) do { if (!(P[i] & PT_FLAG_PRESENT)) return r; } while(0)

struct global_page
{
    uint64_t Physical;
    uint64_t Virtual;
    uint64_t Flags;
};

static struct global_page globalPages[10];
static int gpIndex = 0;

static uint64_t* getNextLevel(uint64_t* level, size_t offset, int flags);
static inline uint64_t readCr3()
{
    uint64_t cr3;
    asm volatile("movq %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void* pgCreateAddressSpace()
{
    uint64_t hhdm = rqGetHhdmRequest()->offset;
    uint64_t* pml4 = mmAlignedAlloc(4096, 4096);
    struct limine_memmap_response* mmap = rqGetMemoryMapRequest();
    struct limine_kernel_address_response* addr = rqGetKernelAddressRequest();
    const struct limine_memmap_entry* ent;

    for (size_t i = 0; i < mmap->entry_count; i++) {
        ent = mmap->entries[i];
        if (ent->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            for (uint64_t ii = 0; ii < ent->length; ii += 4096) {
                pgMapPage(pml4, ent->base + ii, hhdm + ent->base + ii, PT_FLAG_WRITE | PT_FLAG_PCD);
            }
        }

        if (ent->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE
         || ent->type == LIMINE_MEMMAP_ACPI_NVS
         || ent->type == LIMINE_MEMMAP_KERNEL_AND_MODULES
         || ent->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
         || ent->type == LIMINE_MEMMAP_RESERVED) {
            for (uint64_t ii = 0; ii < ent->length; ii += 4096) {
                pgMapPage(pml4, ent->base + ii, hhdm + ent->base + ii, PT_FLAG_WRITE);
            }
         }

    }

    for (uint64_t ii = 0; ii < 5 * MB; ii += 4 * KB) {
        pgMapPage(pml4, addr->physical_base + ii, addr->virtual_base + ii, PT_FLAG_WRITE);
    }

    for (uint64_t i = 0; i < 4 * MB; i += 4 * KB) {
        pgMapPage(pml4, (uint64_t)mmGetGlobal() + i, 
                 (uint64_t)mmGetGlobal() + i, PT_FLAG_WRITE);
    }

    for (int i = 0; i < gpIndex; i++) {
        struct global_page* gp = &globalPages[i];
        pgMapPage(pml4, gp->Physical, gp->Virtual, gp->Flags);
    }

    return pml4;
}

void pgMapPage(address_space_t* addrs, uint64_t paddr, uint64_t vaddr, int flags)
{
    if (!addrs) addrs = (address_space_t*)readCr3();

    flags |= PT_FLAG_PRESENT;

    size_t index4 = (size_t)(vaddr & (0x1FFUL << 39)) >> 39;
    size_t index3 = (size_t)(vaddr & (0x1FFUL << 30)) >> 30;
    size_t index2 = (size_t)(vaddr & (0x1FFUL << 21)) >> 21;
    size_t index1 = (size_t)(vaddr & (0x1FFUL << 12)) >> 12;

    uint64_t* pdpe = getNextLevel(addrs, index4, flags);
    uint64_t* pde = getNextLevel(pdpe, index3, flags);
    uint64_t* pte = getNextLevel(pde, index2, flags);

    pte[index1] = paddr | flags;
    asm volatile ("invlpg (%0)" :: "r"(vaddr) : "memory");
}

void pgUnmapPage(address_space_t* addrs, uint64_t vaddr)
{
    if (!addrs) addrs = (address_space_t*)readCr3();

    size_t index4 = (size_t)(vaddr & (0x1FFUL << 39)) >> 39;
    size_t index3 = (size_t)(vaddr & (0x1FFUL << 30)) >> 30;
    size_t index2 = (size_t)(vaddr & (0x1FFUL << 21)) >> 21;
    size_t index1 = (size_t)(vaddr & (0x1FFUL << 12)) >> 12;

    PAGE_CHECK(addrs, index4);
    uint64_t* pdpe = (uint64_t*)(addrs[index4] & ~511);
    PAGE_CHECK(pdpe, index3);
    uint64_t* pde = (uint64_t*)(pdpe[index3] & ~511);
    PAGE_CHECK(pde, index2);
    uint64_t* pte = (uint64_t*)(pde[index2] & ~511);
    PAGE_CHECK(pte, index1);

    pte[index1] = 0;
    asm volatile ("invlpg (%0)" :: "r"(vaddr) : "memory");
}

void pgSetCurrentAddressSpace(address_space_t* address)
{
    asm volatile("mov %0, %%cr3" :: "r"(address));
}

uint64_t* getNextLevel(uint64_t* level, size_t offset, int flags)
{
    if (!(level[offset] & PT_FLAG_PRESENT)) {
        level[offset] = (uint64_t)(mmAlignedAlloc(4096, 4096));
        memset((uint64_t*)level[offset], 0, 4096);
        level[offset] |= flags;
    }

    level[offset] |= flags;
    return (uint64_t*)(level[offset] & ~511);
}

address_space_t* pgGetAdressSpace() 
{
    return (address_space_t*)readCr3();
}

void* pgGetPhysicalAddress(address_space_t* addrs, uint64_t vaddr)
{
    if (!addrs) addrs = (address_space_t*)readCr3();

    size_t index4 = (size_t)(vaddr & (0x1FFUL << 39)) >> 39;
    size_t index3 = (size_t)(vaddr & (0x1FFUL << 30)) >> 30;
    size_t index2 = (size_t)(vaddr & (0x1FFUL << 21)) >> 21;
    size_t index1 = (size_t)(vaddr & (0x1FFUL << 12)) >> 12;
    size_t offset = (size_t)(vaddr & (0xFFFUL));

    // PAGE_CHECK_RET_VAL(addrs, index4, NULL);
    uint64_t* pdpe = (uint64_t*)(addrs[index4] & ~511);
    // PAGE_CHECK_RET_VAL(pdpe, index3, NULL);
    uint64_t* pde = (uint64_t*)(pdpe[index3] & ~511);
    // PAGE_CHECK_RET_VAL(pde, index2, NULL);
    uint64_t* pte = (uint64_t*)(pde[index2] & ~511);
    // PAGE_CHECK_RET_VAL(pte, index1, NULL);

    return (void*)((pte[index1] & ~511) + offset);
}

void pgAddGlobalPage(uint64_t paddr, uint64_t vaddr, uint64_t flags)
{
    struct global_page* gp = &globalPages[gpIndex];
    gp->Flags = flags;
    gp->Physical = paddr;
    gp->Virtual = vaddr;

    gpIndex++;
    pgMapPage(NULL, paddr, vaddr, flags);
}

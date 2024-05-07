#include "address.h"
#include "arch/i386/paging.h"
#include "memory/physical.h"
#include "term/terminal.h"
#include "mod/ld/elf_ld.h"
#include "utils/vector.h"

#define ADDRESS_DATA_OFFSET 0x40000000000

struct allocated_page {
    uint64_t Offset;
    size_t Length;  
};

struct address_space_mgr 
{
    struct vector Pages;
    address_space_t* AddressSpace;
};

static struct vector cr3vector;

struct address_space_mgr* 
addrGetManagerForCr3(address_space_t* addr)
{
    if (!addr) addr = pgGetAdressSpace();
    struct address_space_mgr* chosen = NULL;

    for (size_t i = 0; i < cr3vector.Length; i++) {
        struct address_space_mgr* d = cr3vector.Data[i];
        if (d->AddressSpace == addr) {
            chosen = cr3vector.Data[i];
        }
    }

    if (!chosen) {
        chosen = mmAllocKernelObject(struct address_space_mgr);
        chosen->AddressSpace = addr;
        
        vectorInsert(&cr3vector, chosen);
        trmLogfn("cr3vector=%p", &cr3vector);
    }
    
    return chosen;
}

bool asmgrClaimPage(struct address_space_mgr* spa, 
                    uint64_t offset, size_t length)
{
    struct allocated_page* pg = NULL;
    for (size_t i = 0; i < spa->Pages.Length; i++) {
        pg = spa->Pages.Data[i];
        if (offset == pg->Offset) {
            return false;
        }

        if (offset > pg->Offset
         && pg->Offset + (pg->Length * 4096) > offset) {
            return false;
        }
    }

    pg = mmAllocKernelObject(struct allocated_page);
    pg->Length = length;
    pg->Offset = offset;
    
    vectorInsert(&spa->Pages, pg);
    return true;
}

uint64_t asmgrCorrectPage(struct address_space_mgr* spa, 
                          uint64_t offset)
{
    struct allocated_page* pg = NULL;
    for (size_t i = 0; i < spa->Pages.Length; i++) {
        pg = spa->Pages.Data[i];

        if (pg->Offset == offset) {
            offset += pg->Length * 4096;
        } 
    }

    return offset;
}

uint64_t asmgrGetDataPage(struct address_space_mgr* spa, size_t length)
{
    /* length might be in non-page aligned value */
    alignUp(length, 4096);
    return asmgrClaimPage(spa, ADDRESS_DATA_OFFSET, length);
}

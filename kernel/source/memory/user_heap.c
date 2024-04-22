#include "user_heap.h"

#include "arch/i386/paging.h"
#include "memory/address.h"
#include "memory/physical.h"
#include "power/system.h"

#define USERVM_BASE_ADDRESS 0x222222200000

static uint64_t mapUser(address_space_t* addrs, uint64_t paddr, 
                        size_t pages, int flags)
{
    __unused(paddr);
    uint64_t fls = PT_FLAG_USER | PT_FLAG_NX;
    if (flags & MAP_USER_WRITE) {
        fls |= PT_FLAG_WRITE;
        fls &= ~(PT_FLAG_NX);
    }

    size_t allocated = pages / 4096;
    struct address_space_mgr* mgr = addrGetManagerForCr3(addrs);
    void* buffer = mmAlignedAlloc(pages, 4096);

    uint64_t beginoffset = asmgrCorrectPage(mgr, USERVM_BASE_ADDRESS);
    asmgrClaimPage(mgr, beginoffset, allocated);
    for (size_t i = 0; i < pages / 4096; i++) {
        pgMapPage(addrs, (uint64_t)buffer, beginoffset + (4096 * i), fls);
    }

    return beginoffset;
}

static void unmapUser(address_space_t* addrs, uint64_t address, size_t size)
{
    void* phys = pgGetPhysicalAddress(addrs, address);
    mmAlignedFree(phys, size);

    for (size_t i = 0; i < size / 4096; i++) {
        pgUnmapPage(addrs, address + (i * 4096));
    }
}

uint64_t syscVirtualUnmap(union sysc_regs* regs)
{
    unmapUser(NULL, regs->Arg1, regs->Arg2);
    return 0;
}

uint64_t syscVirtualMap(union sysc_regs* regs)
{
    return mapUser(NULL, regs->Arg1, regs->Arg2, regs->Arg3);
}



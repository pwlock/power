#include "elf.h"
#include "arch/i386/paging.h"
#include "memory/address.h"
#include "memory/physical.h"
#include "mod/driver/loader.h"
#include "mod/ld/elf_ld.h"
#include "term/terminal.h"
#include "utils/vector.h"

#define EXEC_BASE 0xDCA0000000
#define UME_BASE  0x54000000000

static bool isPic(const struct elf_file_header* he)
{
    bool pic = he->FileType == ELF_FTYPE_SHARED;

    struct elf_program_header* ent;
    struct elf_program_header* ph = PaAdd(he, he->ProgramHeaderTablePosition);
    for (int i = 0; i < he->ProgramHeaderLength; i++) {
        ent = &ph[i];
        if (ent->SegmentType == ELF_PROGTYPE_INTERPRETER) {
            pic = he->FileType == ELF_FTYPE_SHARED;
            break;
        }
    }

    return pic;
}

static uint64_t getExecutableBase(struct address_space_mgr* mgr, 
                                  const struct elf_file_header* he,
                                  int flags)
{
    if (isPic(he)) {
        int pages = 0;
        struct elf_program_header* arr = (struct elf_program_header*)PaAdd(he, he->ProgramHeaderTablePosition);
        for (int i = 0; i < he->ProgramHeaderLength; i++) {
            struct elf_program_header* p = &arr[i];
            if (p->SegmentType == ELF_PROGTYPE_DYNAMIC
             || p->SegmentType == ELF_PROGTYPE_LOAD)
                pages++;
        }

        uint64_t base = (flags & ELF_LOAD_USER_ADDRESS_SPACE) ? UME_BASE : EXEC_BASE;
        uint64_t pg = asmgrCorrectPage(mgr, base);
        if (pg != base) {
            pg += (10 * 4096);
        }

        while (!asmgrClaimPage(mgr, pg, pages)) {
            pg += (10 * 4096);
        }
        
        
        return pg;
    }
    
    return 0;
}

struct elf_executable
modLoadElf(const uint8_t *file, int flags)
{
    return modLoadElfEx(file, NULL, flags);
}

struct elf_executable 
modLoadElfEx(const uint8_t* file, address_space_t* address, int flags)
{
    const struct elf_file_header* he = (const struct elf_file_header*)(file);
    struct elf_executable result;

    if (!he
     || he->Magic[0] != 0x7F
     || he->Magic[1] != 'E'
     || he->Magic[2] != 'L'
     || he->Magic[3] != 'F') {
        result.Result = ELF_LOAD_ERROR_NOT_EXECUTABLE;
        return result;
    }

    if (he->Architecture != 2
     || he->Endianness != 1
     || he->InstructionSet != 0x3E) {
        result.Result = ELF_LOAD_ERROR_WRONG_ARCHITECTURE;
        return result;
    }

    address_space_t* cr3 = address;
    if (flags & ELF_LOAD_NEW_ADDRESS_SPACE)
        cr3 = pgCreateAddressSpace();
    
    if (flags & ELF_LOAD_SHARED_LIB
     || flags & ELF_LOAD_DRIVER) {
        result.LoadedPages = vectorCreate(he->ProgramHeaderLength);
    }

    struct elf_program_header* ent;
    struct elf_program_header* ph;
    ph = (struct elf_program_header*)PaAdd(file, he->ProgramHeaderTablePosition);
    void* program;
    uint64_t pflags;
    struct address_space_mgr* mgr = addrGetManagerForCr3(cr3);
    uint64_t base = getExecutableBase(mgr, he, flags);

    int minoroffset, pages;
    bool pic = isPic(he);

    if (flags & ELF_LOAD_USER_ADDRESS_SPACE
     && !(flags & ELF_LOAD_SHARED_LIB)) {
        modMapDrivers(cr3);
    }

    for (int i = 0; i < he->ProgramHeaderLength; i++) {
        ent = &ph[i];
    
        if (ent->SegmentType != ELF_PROGTYPE_LOAD
         /*&& ent->SegmentType != ELF_PROGTYPE_DYNAMIC*/)
            continue;

        pflags = (flags & ELF_LOAD_USER_ADDRESS_SPACE) ? PT_FLAG_USER : 0;
        if (!(ent->Flags & ELF_PROGFLAGS_EXEC))
            pflags |= PT_FLAG_NX;
        if (ent->Flags & ELF_PROGFLAGS_WRITE)
            pflags |= PT_FLAG_WRITE;

        minoroffset = (ent->VirtualAddress & 0xFFF);
        program = mmAlignedAlloc(ent->MemorySize + minoroffset, ent->Alignment);
        memset(program, 0, ent->MemorySize);
        memcpy(PaAdd(program, minoroffset), PaAdd(he, ent->DataOffset), ent->FileSize);

        pages = ent->MemorySize + minoroffset;
        pages += 4096 - pages % 4096;
        pages /= 4096;
        
        if (!pic)
            asmgrClaimPage(mgr, ent->VirtualAddress, pages);

        struct loaded_page* lp;
        uint64_t paddr, vaddr;
        for (int ii = 0; ii < pages; ii++) {
            paddr = (uint64_t)program + (4096 * ii);
            vaddr = base + ent->VirtualAddress + (4096 * ii);

            if (flags & ELF_LOAD_SHARED_LIB
             || flags & ELF_LOAD_DRIVER) {
                lp = mmAllocKernelObject(struct loaded_page);
                lp->PhysicalAddress = paddr;
                lp->VirtualAddress = vaddr;
                lp->Flags = pflags;

                vectorInsert(&result.LoadedPages, lp);
            }

            pgMapPage(cr3, paddr, vaddr, pflags);
        }
    }

    result.FileHeader = (struct elf_file_header*)he;
    result.Base = base;
    result.EntryPoint = base + (void*)he->EntryPointAddress;
    result.AddressSpace = cr3;
    result.Flags = flags;
    modResolveElfDynamic(&result);
    return result;
}

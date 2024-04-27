#include "elf_ld.h"

#include "arch/i386/paging.h"
#include "config.h"
#include "memory/physical.h"
#include "mod/elf.h"
#include "mod/elf_sym.h"
#include "elf_reloc.h"

#include "term/terminal.h"
#include "utils/string.h"
#include "utils/vector.h"
#include "vfs.h"
#include <stdbool.h>

/* A funny fact: No executable knows which modules it loaded.
   (TODO) */

static struct vector loadedLibraries;

static struct elf_program_header*
getDynamicProg(const struct elf_executable* exe)
{
    struct elf_program_header* parr = PaAdd(exe->FileHeader,
                                            exe->FileHeader->ProgramHeaderTablePosition);
    for (int i = 0; i < exe->FileHeader->ProgramHeaderLength; i++) {
        struct elf_program_header* prog = &parr[i];
        if (prog->SegmentType == ELF_PROGTYPE_DYNAMIC)
            return prog;
    }

    return NULL;
}

static bool wasLoaded(address_space_t* addrs, struct loaded_library* lib)
{
    for (size_t i = 0; i < lib->LoadedAddresses.Length; i++) {
        address_space_t* d = lib->LoadedAddresses.Data[i];
        if (d == addrs)
            return true;
    }

    return false;
}

/* Load library *library* onto address space of *exe*. */
static void mapLibrary(const struct elf_executable* exe, struct loaded_library* library)
{
    if (!wasLoaded(exe->AddressSpace, library)) {
        for (size_t i = 0; i < library->Pages.Length; i++) {
            struct loaded_page* pg = library->Pages.Data[i];
            pgMapPage(exe->AddressSpace, pg->PhysicalAddress,
                      pg->VirtualAddress, pg->Flags);
        }
        vectorInsert(&library->LoadedAddresses, exe->AddressSpace);
    }

    elfRelocateRelative(exe, ".rela.dyn");
    elfRelocateAbsoluteSymbols(exe, library, ".rela.dyn");
    elfRelocateAbsoluteSymbols(exe, library, ".rela.plt");
}

static void createLibrary(const struct elf_executable* exe, const char* name)
{
    size_t pathlen = strlen("A:/System/Libraries/");
    char* actualName = mmAllocKernel(strlen(name) + pathlen);
    memcpy(actualName, "A:/System/Libraries/", pathlen);
    memcpy(actualName + pathlen, name, strlen(name));

    struct fs_file* f = vfsOpenFile(actualName, 0);
    uint8_t* buffer = mmAllocKernel(f->Size);
    vfsReadFile(f, buffer, f->Size);
    struct elf_executable lib = modLoadElfEx(buffer, exe->AddressSpace,
                                             ELF_LOAD_USER_ADDRESS_SPACE | ELF_LOAD_SHARED_LIB);
    vfsCloseFile(f);

    struct loaded_library* lload = mmAllocKernelObject(struct loaded_library);
    lload->Name = mmAllocKernel(strlen(name));
    lload->Pages = lib.LoadedPages;
    lload->File = lib.FileHeader;
    lload->Base = lib.Base;
    lload->LoadedAddresses = vectorCreate(1);
    lload->ReferenceCount = 1;

    mapLibrary(exe, lload);
    memcpy(lload->Name, name, strlen(name));
    vectorInsert(&loadedLibraries, lload);
    trmLogfn("lload->Name=%s, lload->Base=%p", lload->Name, lload->Base);
    mmAlignedFree(actualName, strlen(name) + 20);
}

static void loadLibrary(const struct elf_executable* exe, const char* name)
{
    if (!strncmp(name, "lib__r-kernelapi.so", strlen(name))
        && !(exe->Flags & ELF_LOAD_USER_ADDRESS_SPACE)) {
        return;
    }

    for (size_t i = 0; i < loadedLibraries.Length; i++) {
        struct loaded_library* ll = loadedLibraries.Data[i];
        if (!strncmp(name, ll->Name, strlen(name))) {
            ll->ReferenceCount++;
            mapLibrary(exe, ll);
            return;
        }
    }

    createLibrary(exe, name);
}

static void fixSymbols(const struct elf_executable* exec,
                       struct elf_rela_item* item)
{
    if ((exec->Flags & ELF_LOAD_USER_ADDRESS_SPACE))
        return;

    const struct elf_section_header* sec = modGetSection(exec, ".rela.plt");
    const struct elf_section_header* dynstrsec = modGetSection(exec, ".dynstr");
    const struct elf_section_header* dynsymsec = modGetSection(exec, ".dynsym");
    const struct elf_symtab_item* dynsym = PaAdd(exec->FileHeader, dynsymsec->FileOffset);
    const char* dynstr = PaAdd(exec->FileHeader, dynstrsec->FileOffset) + 1;

    void* symbol;
    int length = sec->SectionSize / sec->TableEntrySize;

    const struct elf_symtab_item* symt;
    const struct elf_rela_item* ii;

    for (int i = 0; i < length; i++) {
        ii = &item[i];
        uint64_t sym = ELF64_R_SYM(ii->Info);

        symt = &dynsym[sym];
        if (!(exec->Flags & ELF_LOAD_USER_ADDRESS_SPACE)) {
            symbol = modGetSymbolElf(NULL, dynstr + symt->NameOffset - 1);
            void* ptr = pgGetPhysicalAddress(exec->AddressSpace, exec->Base + ii->Offset);
            *(uint64_t*)(ptr) = (uint64_t)symbol;
        }
    }
}

const struct elf_section_header*
modGetSection(const struct elf_executable* exe, const char* name)
{
    const struct elf_file_header* header = exe->FileHeader;
    const struct elf_section_header* secs = PaAdd(header, header->SectionHeaderTablePosition);
    const struct elf_section_header* str = &secs[header->NamesSectionIndex];
    const char* shstrtab = (const char*)(PaAdd(exe->FileHeader, str->FileOffset));

    for (int i = 0; i < header->ProgramSectionLength; i++) {
        const struct elf_section_header* s = &secs[i];
        if (!strncmp(shstrtab + s->NameOffset, name, strlen(name))) {
            return s;
        }
    }

    return NULL;
}

void modResolveElfDynamic(const struct elf_executable* exe)
{
    static bool createdVector = false;
    if (!createdVector) {
        loadedLibraries = vectorCreate(2);
        createdVector = true;
    }
    uint64_t* gotptr;

    const struct elf_section_header* strt = modGetSection(exe, ".dynstr");
    const char* strtab = (char*)(exe->Base + strt->Address);

    const struct elf_section_header* got = modGetSection(exe, ".got.plt");
    if (!got) /* Its a statically linked exec! */
        return;

    struct elf_program_header* dyn = getDynamicProg(exe);
    struct elf_dynamic_item* darr = (struct elf_dynamic_item*)(PaAdd(exe->FileHeader, dyn->DataOffset));
    const struct elf_section_header* relaplt = modGetSection(exe, ".rela.plt");
    fixSymbols(exe, (struct elf_rela_item*)(exe->Base + relaplt->Address));

    if (exe->Flags & ELF_LOAD_USER_ADDRESS_SPACE)
        strtab = PaAdd(exe->FileHeader, strt->FileOffset);

    while (darr->Type != 0x0) {
        switch (darr->Type) {
        case 0x1: /* NEEDED */
            loadLibrary(exe, strtab + darr->Value);
            break;
        case 0x7: /* RELA */
            break;
        }
        darr++;
    }
}

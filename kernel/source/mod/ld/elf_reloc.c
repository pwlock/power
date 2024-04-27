#include "elf_reloc.h"
#include "memory/physical.h"
#include "mod/elf_sym.h"
#include "mod/ld/elf_ld.h"
#include "term/terminal.h"

void elfRelocateAbsoluteSymbols(const struct elf_executable* exe, 
                                struct loaded_library* library, 
                                const char* section)
{
    /* GetSection only accepts a elf_executable, so we have to synthetize one.
       This is not an issue, given that function only uses the FileHeader field.
       I could, theoretically, just change the type of the 1st argument, but
       ain't no way I be doin' dat. */
    struct elf_executable fakelibexe = { library->File, NULL, NULL, 0, 0, 0, {} };
    fakelibexe.Base = library->Base;

    const struct elf_section_header* dynsymsec = modGetSection(exe, ".dynsym");
    const struct elf_section_header* dynstrsec = modGetSection(exe, ".dynstr");
    const struct elf_symtab_item* dynsym = PaAdd(exe->FileHeader, dynsymsec->FileOffset);
    const char* dynstr = PaAdd(exe->FileHeader, dynstrsec->FileOffset);
    
    const struct elf_section_header* sec = modGetSection(exe, section);
    trmLogfn("section %s = %p (%p)", section, sec, exe->Base);
    if (!sec)
        return;

    const struct elf_rela_item* relaplt = PaAdd(exe->FileHeader, sec->FileOffset);
    int length = sec->SectionSize / sec->TableEntrySize;

    void* ptr;
    void* symbol;
    const char* symn;
    const struct elf_symtab_item* symt;
    const struct elf_rela_item* item;

    for (int i = 0; i < length; i++) {
        item = &relaplt[i];
        int type = ELF64_R_TYPE(item->Info);
        if (type != ELF64_R_X86_64_GLOB_DAT
          && type != ELF64_R_X86_64_JUMP_SLOT)
            continue;

        uint64_t sym = ELF64_R_SYM(item->Info);

        symt = &dynsym[sym];
        symn = dynstr + symt->NameOffset;

        void* orig = modGetSymbolElf(&fakelibexe, symn);
        if (!orig) {
            continue;
        }

        symbol = PaAdd(orig, library->Base);
        ptr = pgGetPhysicalAddress(exe->AddressSpace, exe->Base + item->Offset);
        *(uint64_t*)(ptr) = (uint64_t)symbol;
    }
}

void elfRelocateRelative(const struct elf_executable* exe,
                         const char* section)
{
    const struct elf_section_header* sec = modGetSection(exe, section);
    if (!sec)
        return;
    
    void* ptr;
    const struct elf_rela_item* item;
    const struct elf_rela_item* relaplt = PaAdd(exe->FileHeader, sec->FileOffset);
    int length = sec->SectionSize / sec->TableEntrySize;

    for (int i = 0; i < length; i++) {
        item = &relaplt[i];
        switch (ELF64_R_TYPE(item->Info)) {
        case ELF64_R_X86_64_GLOB_DAT:
        case ELF64_R_X86_64_JUMP_SLOT:
            /* They will be handled by AbsoluteSymbols. */
            continue;
        case ELF64_R_X86_64_RELATIVE:
            ptr = pgGetPhysicalAddress(exe->AddressSpace, exe->Base + item->Offset);
            trmLogfn("item addend = %p", item->Addend);
            *(uint64_t*)(ptr) = exe->Base + item->Addend;
            break;
        }
    }
}

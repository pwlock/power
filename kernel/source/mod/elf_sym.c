#include "elf_sym.h"

#include "bootloader_requests.h"
#include "memory/physical.h"
#include "mod/elf.h"
#include "mod/ld/elf_ld.h"
#include "mod/ld/elf_hashsym.h"
#include "term/terminal.h"
#include "utils/string.h"

#define min(X, y) ((X) > (y)) ? (y) : (X)

static struct elf_executable kernelFile = {};

bool elfSlowCompare(const char* s1, const char* s2)
{
    while (*s1 || *s2) {
        if (*s1 != *s2)
            return false;
        s1++; s2++;
    }
    return true;
}

void* modGetSymbolElf(struct elf_executable* exec, const char* symbol)
{
    kernelFile.FileHeader = rqGetKernelFileResponse()->kernel_file->address;
    if (!exec)
        exec = &kernelFile;
    else
        return modSearchHashSymbol(exec, symbol);
    
    const char* symtabname = (exec == &kernelFile) ? ".symtab" : ".dynsym";
    const char* strtabname = (exec == &kernelFile) ? ".strtab" : ".dynstr";

    struct elf_symtab_item* symtab;
    const struct elf_section_header* symtabsec = modGetSection(exec, symtabname);
    const struct elf_section_header* strtabsec = modGetSection(exec, strtabname);

    symtab = (struct elf_symtab_item*)PaAdd(exec->FileHeader, symtabsec->FileOffset);
    const char* strtab = PaAdd(exec->FileHeader, strtabsec->FileOffset);

    size_t stl = strlen(symbol);
    uint64_t length = symtabsec->SectionSize / symtabsec->TableEntrySize;

    if (elfSlowCompare(symbol, "schedCreateThread"))
        trmLogfn("symbol=%s", symbol);
    for (uint64_t i = 1; i < length; i++) {
        const struct elf_symtab_item* s = &symtab[i];
        size_t symtl = strlen(strtab + s->NameOffset);

        if (elfSlowCompare(symbol, strtab + s->NameOffset)) {
            return (void*)(s->Value);
        }
    }

    return NULL;
}

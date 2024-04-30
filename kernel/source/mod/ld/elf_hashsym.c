#include "elf_hashsym.h"
#include "memory/physical.h"
#include "mod/elf.h"
#include "mod/ld/elf_ld.h"
#include "mod/elf_sym.h"
#include "term/terminal.h"
#include "utils/string.h"

#include <stdint.h>

static unsigned long 
oldStyleHash(const char* name)
{
    unsigned long h = 0;
    unsigned long g;

    while(*name) {
        h = (h << 4) + *name++;
        if ((g = h & 0xf0000000)) {
            h ^= g >> 24;
        }
        h &= 0x0fffffff;
    }

    return h;
}

void* modSearchHashSymbol(struct elf_executable* exec, 
                          const char* symbolName) 
{
    const struct elf_section_header* es = modGetSection(exec, ".hash");
    const struct elf_section_header* ds = modGetSection(exec, ".dynsym");
    const struct elf_section_header* dstr = modGetSection(exec, ".dynstr");

    const struct elf_symtab_item* dsym = PaAdd(exec->FileHeader, ds->FileOffset);
    const char* dynstr = PaAdd(exec->FileHeader, dstr->FileOffset);
    const struct elf_hash* hs = PaAdd(exec->FileHeader, es->FileOffset);

    const uint32_t* bucket = PaAdd(hs, sizeof(*hs));
    const uint32_t* chain = PaAdd(bucket, hs->BucketCount * sizeof(uint32_t));
    uint64_t hash = oldStyleHash(symbolName);

    for (uint32_t i = bucket[hash % hs->BucketCount]; i; i = chain[i]) {
        if (oldStyleHash(dynstr + dsym[i].NameOffset) != hash) {
            continue;
        }

        if (elfSlowCompare(symbolName, dynstr + dsym[i].NameOffset)) {
            if (dsym[i].SectionHeaderTableIndex == ELF_SHN_UNDEF)
                return NULL;

            return (void*)dsym[i].Value;
        }
    }

    return NULL;
}

#include "elf_hashsym.h"
#include "memory/physical.h"
#include "mod/elf.h"
#include "mod/ld/elf_ld.h"
#include "mod/elf_sym.h"
#include "term/terminal.h"
#include "utils/string.h"

#include <stdint.h>

static uint32_t oldStyleHash(const char* name)
{
    uint32_t h = 0;
    uint32_t g;

    while(*name) {
        h = (h << 4) + *name++;
        if ((g = h & 0xf0000000)) {
            h ^= g >> 24;
        }

        h &= ~g;
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
    uint32_t hash = oldStyleHash(symbolName);
    
    for (uint32_t i = bucket[hash % hs->BucketCount]; i; i = chain[i]) {
        if (oldStyleHash(dynstr + dsym[i].NameOffset) != hash) {
            continue;
        }

        if (!strcmp(symbolName, dynstr + dsym[i].NameOffset)) {
            if (dsym[i].SectionHeaderTableIndex == ELF_SHN_UNDEF)
                return NULL;

            return (void*)dsym[i].Value;
        }
    }

    return NULL;
}

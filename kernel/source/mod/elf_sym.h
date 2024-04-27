/* elf_sym.h
   Purpose: ELF symbol loading */
#pragma once

#include "mod/elf.h"

struct elf_symtab_item 
{
	uint32_t NameOffset;
	uint8_t Info;
	uint8_t Other;
	uint16_t SectionHeaderTableIndex;
	uint64_t Value;
	uint64_t Size;
};

/* NULL will make the function search in the kernel
   symtab */
void* modGetSymbolElf(struct elf_executable*, const char* symbol);

bool elfSlowCompare(const char* s1, const char* s2);

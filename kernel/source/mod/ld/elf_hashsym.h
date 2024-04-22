/* elf_hashsym.h
   Purpose: ELF symbol lookup through hash */
#pragma once

#include "mod/elf.h"
#include <stdint.h>

struct elf_hash
{
    uint32_t BucketCount;
    uint32_t ChainCount;
};

void* modSearchHashSymbol(struct elf_executable* exec, const char* symbolName);

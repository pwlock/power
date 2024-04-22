/* elf_ld.h
   Purpose: The dynamic linker for ELF files */
#pragma once

#include "mod/elf.h"
#include "utils/vector.h"

#define alignUp(P, l) P + l - P % l;

struct loaded_library
{
    char* Name;
    struct elf_file_header* File;
    uint64_t Base;
    struct vector Pages;
    struct vector LoadedAddresses; /*< Address spaces this library was loaded inside. */
    int ReferenceCount; /*< Number of times this library was loaded */
};

struct loaded_page
{
    uint64_t VirtualAddress;
    uint64_t PhysicalAddress;
    uint64_t Flags;
};

void modResolveElfDynamic(const struct elf_executable*);
const struct elf_section_header* 
    modGetSection(const struct elf_executable* exe, const char* name);
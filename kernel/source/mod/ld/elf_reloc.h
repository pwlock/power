/* elf_reloc.h
   Purpose: ELF relocation */
#pragma once

#include "mod/elf.h"
#include "mod/ld/elf_ld.h"

#define ELF64_R_X86_64_GLOB_DAT 6
#define ELF64_R_X86_64_JUMP_SLOT 7
#define ELF64_R_X86_64_RELATIVE 8

/* Macros from the ELF-64 Object File Format document.
   <https://uclibc.org/docs/elf-64-gen.pdf> */
#define ELF64_R_SYM(I) ((I) >> 32)
#define ELF64_R_TYPE(I) ((I) & 0xffffffffL)
#define ELF64_R_INFO(S, t) (((S) << 32) + ((t) & 0xffffffffL))


/* Relocate X86_64_JUMP_SLOT and X86_64_GLOB_DAT symbols
   located inside relocation section `section` 
   against library `lib` */
void elfRelocateAbsoluteSymbols(const struct elf_executable* exec, 
                                struct loaded_library* lib, 
                                const char* section);

/* Perform other relocations which are not covered by AbsoluteSymbols. */
void elfRelocateRelative(const struct elf_executable* exec, const char* section);

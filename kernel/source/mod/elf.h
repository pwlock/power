/* elf.h
   Purpose: The ELF loader */
#pragma once

#include "arch/i386/paging.h"
#include "config.h"
#include "utils/vector.h"
#include <stdint.h>

#define ELF_FTYPE_RELOC 1
#define ELF_FTYPE_EXEC  2
#define ELF_FTYPE_SHARED 3
#define ELF_FTYPE_CORE   4

struct __attribute__((packed)) 
elf_file_header
{
    uint8_t Magic[4];
    uint8_t Architecture;
    uint8_t Endianness;
    uint8_t HeaderVersion;
    uint8_t Abi;
    uint64_t Padding;

    uint16_t FileType;
    uint16_t InstructionSet;
    uint32_t FileVersion;
    uint64_t EntryPointAddress;
    uint64_t ProgramHeaderTablePosition;
    uint64_t SectionHeaderTablePosition;
    uint32_t Flags;

    uint16_t HeaderSize;
    uint16_t ProgramHeaderSize;
    uint16_t ProgramHeaderLength;
    uint16_t ProgramSectionSize;
    uint16_t ProgramSectionLength;
    uint16_t NamesSectionIndex;
};

#define ELF_PROGTYPE_NULL 0
#define ELF_PROGTYPE_LOAD 1
#define ELF_PROGTYPE_DYNAMIC 2
#define ELF_PROGTYPE_INTERPRETER 3
#define ELF_PROGTYPE_NOTE        4

#define ELF_PROGFLAGS_EXEC 1
#define ELF_PROGFLAGS_WRITE (1 << 1)
#define ELF_PROGFLAGS_READ  (1 << 2)

struct __attribute__((packed)) 
elf_program_header
{
    uint32_t SegmentType;
    uint32_t Flags;
    uint64_t DataOffset;
    uint64_t VirtualAddress;
    uint64_t Reserved; /*< Some ABIs define this as the
                          physical address. */
    uint64_t FileSize;
    uint64_t MemorySize;
    uint64_t Alignment;
};

#define ELF_SHN_UNDEF 0

struct __attribute__((packed))
elf_section_header 
{
    uint32_t NameOffset;
    uint32_t SectionType;
    uint64_t Flags;
    uint64_t Address;
    uint64_t FileOffset;
    uint64_t SectionSize;
    uint32_t SectionLink;
    uint32_t Info;
    uint64_t AddressAlignment;
    uint64_t TableEntrySize;
};

struct __attribute__((packed))
elf_rela_item 
{
    uint64_t Offset;
    uint64_t Info;
    uint64_t Addend;
};

struct __attribute__((packed))
elf_dynamic_item
{
    int64_t Type;
    uint64_t Value; /*< or Address. */
};

/* Create a new address space for the file */
#define ELF_LOAD_NEW_ADDRESS_SPACE 0x00001

/* The created address space is readable by CPL 3. */
#define ELF_LOAD_USER_ADDRESS_SPACE 0x00010

/* The loaded executable is a driver. */
#define ELF_LOAD_DRIVER             0x00100

/* Load a shared library (aka include the created pages
   inside the returned structure) */
#define ELF_LOAD_SHARED_LIB         0x10000

#define ELF_LOAD_ERROR_OK             0 /* File successfully loaded */
#define ELF_LOAD_ERROR_NOT_EXECUTABLE 1 /* Not an executable */
#define ELF_LOAD_ERROR_WRONG_ARCHITECTURE 2 /* Architecture/ISA mismatch */

struct elf_executable 
{
    struct elf_file_header* FileHeader;
    address_space_t* AddressSpace;
    void* EntryPoint;
    uint64_t Base;
    int Result;
    int Flags;

    struct vector LoadedPages;
};

struct elf_executable modLoadElf(const uint8_t* file, int flags);
struct elf_executable modLoadElfEx(const uint8_t* file, address_space_t* address, int flags);
void modFreeElf(struct elf_executable*);

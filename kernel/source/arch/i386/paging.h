/* paging.h
   Purpose: Virtual memory manager */
#pragma once

#include <stddef.h>
#include <stdint.h>

#define KB 1024
#define MB KB * 1024
#define GB MB * 1024

#define PT_FLAG_PRESENT  (1UL << 0)
#define PT_FLAG_WRITE    (1UL << 1)
#define PT_FLAG_USER     (1UL << 2)
#define PT_FLAG_PCD      (1UL << 4)
#define PT_FLAG_LARGE    (1UL << 7)
#define PT_FLAG_GUARD    (1UL << 9)
#define PT_FLAG_NX       (1UL << 63)

typedef uint64_t address_space_t;

void* pgCreateAddressSpace();
void pgSetCurrentAddressSpace(address_space_t* address);
void pgMapPage(address_space_t* addrs, uint64_t paddr, uint64_t vaddr, int flags);
void pgUnmapPage(address_space_t* addrs, uint64_t vaddr);

uint64_t pgMapUser(address_space_t* addrs, uint64_t paddr, size_t pages, int flags);

void pgAddGlobalPage(uint64_t paddr, uint64_t vaddr, uint64_t flags);
void* pgGetPhysicalAddress(address_space_t* addrs, uint64_t vaddr);
address_space_t* pgGetAdressSpace();

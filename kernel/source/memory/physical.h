/* physical.h
   Purpose: Physical memory allocator */
#pragma once

#include "config.h"

#define MALL_NO_MAPPING 1 /*< Do not map the buffer. */

#define PaAdd(B, s) (void*)(((uint64_t)B) + s)
#define mmAllocKernelObject(O) mmAlignedAlloc(sizeof(O), 1)
#define mmFreeKernelObject(P) mmAlignedFree((P), sizeof(typeof(*(P))))

#define mmAllocKernel(S) mmAlignedAlloc((S), 1)

void mmInit(void* base, size_t length);
void mmAddUsablePart(void* begin, size_t length);

void* mmAlignedAlloc(size_t size, uint16_t alignment);
void mmAlignedFree(const void* ptr, size_t size);

void* mmGetGlobal();
size_t mmGetLength();

bool mmIsPhysical(void* ptr);

/* Function defined in assembly */
extern void* memset(void* ptr, int value, size_t size);
extern void* _64memset(void* ptr, uint64_t value, size_t qsize);
extern void* memcpy(void* restrict dst, const void* restrict src, size_t length);

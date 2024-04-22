/* address.h
   Purpose: virtual address allocation */
#pragma once

#include "config.h"
#include "arch/i386/paging.h"

struct address_space_mgr* addrGetManagerForCr3(address_space_t* addr);
bool asmgrClaimPage(struct address_space_mgr*, uint64_t offset, size_t length);

uint64_t asmgrCorrectPage(struct address_space_mgr*, uint64_t offset);
uint64_t asmgrGetDataPage(struct address_space_mgr*, size_t length);

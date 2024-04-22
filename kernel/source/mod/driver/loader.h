/* loader.h 
   Purpose: Driver loading */
#pragma once

#include "arch/i386/paging.h"
#include "config.h"
#include "mod/driver/driver_kinterface.h"
#include "utils/vector.h"

struct driver_internal_data
{
    struct kdriver_manager* Driver;
    struct vector Pages; /*< For mapping onto user address space 
                             of created processes*/
};

struct kdriver_manager* modGetDriver(const char* modName);
struct kdriver_manager* modGetFilesystemName(const char* fsName);
struct kdriver_manager* modLoadDriver(const char* modName);
struct kdriver_manager* modLoadDriverEx(const char* modName, bool insideRamfs);
void modMapDrivers(address_space_t* address);

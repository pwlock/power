/* ustar.h
   Purpose: Load files located 
          inside POSIX tar files. */
#pragma once

#include "config.h"

int modLoadUstarFile(const int8_t* archive, const char* name, 
                     const uint8_t** buffer);

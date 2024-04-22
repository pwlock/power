/* translation.h
   Purpose: translaton from PS/2 cojoined scan codes 
        to the macros defined in power/input_code.h */
#pragma once

#include <stdint.h>

uint32_t ps2TranslateScanCode2Key(uint64_t sc);

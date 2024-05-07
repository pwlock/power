/* config.h
   Purpose: macros used throught the kernel */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define __barrier __sync_synchronize

#define __unused(X) ((void)X)

#define __min(X, y) ((X) > (y)) ? (y) : (X)
#define __max(X, y) ((X) > (y)) ? (X) : (y)

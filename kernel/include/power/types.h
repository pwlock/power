/* types.h
   Purpose: Type definitions used inside 
            user kernel headers */
#pragma once

#ifndef _POWER_TYPES_H
#define _POWER_TYPES_H

typedef signed char __char;
typedef unsigned char __uchar;

typedef short __int16;
typedef unsigned short __uint16;

typedef int __int32;
typedef unsigned int __uint32;

typedef long __int64;
typedef unsigned long __uint64;

typedef __int32 __handle;

#if defined(__GNUC__) || defined(__clang__)
#define __kpacked __attribute__((packed))
#else
#define __kpacked
#endif

#endif


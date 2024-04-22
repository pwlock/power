/* system.h
   Purpose: Kernel API */
#pragma once

/* System call slots */
#define SYS_OPENFILE     1
#define SYS_WRITEFILE    2
#define SYS_READFILE     3
#define SYS_CLOSEFILE    4
#define SYS_GETCOMMANDLINE 5
#define SYS_EXITPROCESS    6
#define SYS_MAPPAGES       7
#define SYS_UNMAPPAGES     8
#define SYS_SETFILEPOINTER 9
#define SYS_DEVICEIOCTL    10
#define SYS_MOUNTVOLUME    11

/* Flags for MapPages */
#define MAP_USER_WRITE 0x100
#define MAP_USER_EXECUTE 0x010
#define MAP_ANONYMOUS    0x001

/* Flags for OpenFile */
#define OPENFILE_READ  0x0000001 /*< The file was open for read */
#define OPENFILE_WRITE 0x0000010 /*< The file was open for write */

/* Flags for SetFilePointer */
#define SFILEPTR_REL_BEGIN 0
#define SFILEPTR_REL_END   1
#define SFILEPTR_REL_CURRENT 2

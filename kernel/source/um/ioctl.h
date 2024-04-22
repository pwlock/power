/* ioctl.h
   Purpose: DeviceIoctl system call definition */
#pragma once

#include "syscall.h"

uint64_t syscIoControl(union sysc_regs* regs);

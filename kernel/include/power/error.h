/* error.h
   Purpose: Definitions of error codes for syscalls */
#pragma once

#define ERROR_OK         0 /*< Everything went OK. */
#define ERROR_NOT_FOUND  1 /*< Resource not found. */
#define ERROR_NO_SYSCALL 2 /*< Unknown syscall */
#define ERROR_NO_DEVICE  3 /*< The device is empty. */
#define ERROR_TOO_BIG    4 /*< The value does not fall within the expected range.*/
#define ERROR_FORBIDDEN  5 /*< No permission to access resource. */
#define ERROR_INVALID_ARGUMENT 6 /*< An argument was invalid. */
#define ERROR_TIMED_OUT        7 /*< Timed out while waiting for resource. */
#define ERROR_IO               8 /*< An physical I/O error has occured. */

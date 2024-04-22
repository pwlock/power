/* string.h
   Purpose: String-related utilities */
#pragma once

#include <stddef.h>

int strncmp(const char* s1, const char* s2, size_t length);
__attribute__((pure)) size_t strlen(const char* s);
int strcmp(const char* s1, const char* s2);

void sscanf(const char* string);

#include "string.h"

size_t strlen(const char* str)
{
    const char* s = str;
    while (*str != 0) ++str;
    return ((size_t)(str - s));
}

int strncmp(const char *s1, const char *s2, size_t length)
{
    size_t l = 0;
    while (l < length) {
        if (s1[l] < s2[l])
            return -1;
        if (s1[l] > s2[l])
            return 1;
        l++;
    }

    return 0;
}

int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1 == *s2)) s1++; s2++;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}


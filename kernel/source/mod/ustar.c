#include "ustar.h"
#include "term/terminal.h"
#include "utils/string.h"

static int octInteger(const char* str, int size)
{
    int c = 0;
    for (int i = 0; i < size; i++) {
        char w = str[i];
        c *= 8;
        c += w - '0';
    }
    return c;
}

int modLoadUstarFile(const int8_t* archive, const char* name, 
                     const uint8_t** buffer)
{
    if (!archive) 
        return -1;

    int size;
    while (!strncmp((const char*)archive + 257, "ustar", 5)) {
        size = octInteger((const char*)archive + 0x7C, 11);
        if (!strncmp((const char*)archive, name, strlen(name))) {
            (*buffer) = (const uint8_t*)archive + 512;
            return size;
        }

        archive += (((size + 511) / 512) + 1) * 512;
    }

    return 0;
}

#include "cmd.h"
#include "bootloader_requests.h"
#include "utils/string.h"

static const char* getOption(const char* command, const char* name)
{
    const char* ptr = command;
    while(*ptr) {
        if (ptr == command || *ptr == ' ') {
            int addend = *ptr == ' ' ? 1 : 0;
            if (!strncmp(name, ptr + addend, strlen(name))) {
                return ptr + addend;
            }
        }

        ptr++;
    }

    return NULL;
}

size_t cmdGetCommandArgument(const char* name, const char** output)
{
    struct limine_file* file = rqGetKernelFileResponse()->kernel_file;
    const char* opt = getOption(file->cmdline, name);
    if (!opt)
        return 0;

    bool foundEqual = false;
    const char* ptr = opt;
    const char* begin = opt;
    while(*ptr && *ptr != ' ') {
        if (*ptr == '=' && !foundEqual) {
            foundEqual = true;
            (*output) = ptr + 1;
            begin = ptr + 1;
        }

        ptr++;
    }

    return ptr - begin;
}

bool cmdHasArgument(const char* name)
{
    struct limine_file* file = rqGetKernelFileResponse()->kernel_file;
    return getOption(file->cmdline, name) != NULL;
}

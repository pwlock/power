#include "autoloader.h"
#include "bootloader_requests.h"
#include "config.h"
#include "memory/physical.h"
#include "mod/driver/loader.h"
#include "mod/ustar.h"
#include "term/terminal.h"

void modAutomaticLoad()
{
    char* temp = mmAllocKernel(32);
    const uint8_t* buffer;
    struct limine_module_response* m = rqGetModuleResponse();
    struct limine_file* f = m->modules[0];

    modLoadUstarFile(f->address, "driver.autoload", &buffer);
    const char* ptr = (const char*)buffer;
    const char* begin = ptr;
    while(true) {
        if (*ptr == '\n'
         || *ptr == '\0') {
            memset(temp, 0, 32);
            if (ptr - begin > 32) {
                trmLogfn("ignoring big filename inside driver.autoload");
                begin = ptr + 1;
                if (!*ptr)
                    break;
                continue;
            }

            memcpy(temp, begin, ptr - begin);
            trmLogfn("loading driver %s", temp);
            if (*temp) 
                modLoadDriver(temp);
            begin = ptr + 1;
            if (!*ptr)
                break;
        }

        ptr++;
    }
    
    mmAlignedFree(temp, 32);
}

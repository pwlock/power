#include "mount.h"

#include "config.h"
#include "driver.h"
#include "susp.h"
#include "vfs.h"
#include "isodef.h"

#include "memory/physical.h"
#include "mod/ld/elf_ld.h"
#include "term/terminal.h"
#include "vfs.h"

#include "utils/string.h"

static void upperString(const char* string, char* output)
{
    while(*string) {
        if (*string >= 'a' && *string <= 'z') {
            *output = *string - 0x20;
        } else {
            *output = *string;
        }

        output++;
        string++;
    }
}

struct driver_mounted_fs_interface*
isofsMount(struct isofs* iso, struct driver_disk_device_interface* dev, lba_t beginLba,
           struct fs_mountpoint* mp)
{
    struct mounted_isofs* mounted = NULL;
    uint8_t* buffer = mmAlignedAlloc(800, 1);
    lba_t lba = beginLba + 16;

    while (true) {
        dev->readSector(dev, lba, 800, buffer);

        switch (*buffer) {
        case 0x01:
            iso->Volume = *(struct iso9660_primary_vol*)buffer;
            break;
        case 0xFF:
            trmLogfn("Primary volume not found");
            goto end;
        }

        if (*buffer == 0x01)
            break;
        lba++;
    }

    mounted = (struct mounted_isofs*)createMountedIso(iso, dev, mp);

end:
    mmAlignedFree(buffer, 800);
    return (struct driver_mounted_fs_interface*)mounted;
}

void isofsCreateFile(struct mounted_isofs* mount, 
                     const char *path, struct fs_file* outFile)
{
    struct iso9660_directory_record* d = &mount->RootDirectory;
    struct iso9660_directory_record* rec = (struct iso9660_directory_record*)mount->ScratchBuffer;
    struct iso9660_directory_record* recptr = rec;
    mount->Device->readSector(mount->Device, d->ExtentLba.Little, 2048, mount->ScratchBuffer);

    size_t recsize = 2048;
    size_t length = strlen(path) + 1;
    size_t i = 0;
    const char* ptr = path;
    const char* begin = ptr;

    size_t sofrec;
    struct susp_new_state ns;
    while(i < length) {
        if (*ptr == '/' || *ptr == '\0') {
            bool found = false;
            while (recsize > sizeof(struct iso9660_directory_record)) {
                if (!recptr->Size)
                    break;
                
                const char* name = recname(recptr);
                sofrec = sizeofrec(recptr);
                if (sofrec != recptr->Size) {
                    ns = suspHandleEntry(recptr);
                    if (ns.FileNameLength > 0) {
                        name = ns.FileName;
                    }
                }

                if (!strncmp(name, begin, ptr - begin)) {
                    found = true;
                    break;
                }

                recsize -= recptr->Size;
                recptr = (struct iso9660_directory_record*)(((uint8_t*)recptr) + recptr->Size);
            }
            
            begin = ptr + 1;
            if (!found) {
                outFile->Bad = FS_ERROR_NOT_FOUND;
                return;
            }

            if (*ptr == '\0') {
                /* Found the file, now fill the handle. */
                outFile->Reserved = mmAlignedAlloc(recptr->Size, 1);
                memcpy(outFile->Reserved, recptr, recptr->Size);
                outFile->Size = recptr->DataLength.Little;
                return;
            }

            if (recptr->Flags & (1 << 1)) {
                /* the component is a directory, enter it. */
                mount->Device->readSector(mount->Device, recptr->ExtentLba.Little, 
                                          2048, mount->ScratchBuffer);

                recptr = (struct iso9660_directory_record*)mount->ScratchBuffer;
                recsize = 2048;
            }
        }

        i++;
        ptr++;
    }
}

int isofsReadFile(struct mounted_isofs* mount, uint8_t* buffer, 
                   size_t size, struct fs_file* file)
{
    if (!file->Reserved) {
        trmLogfn("No file to be read.");
        return -1;
    }
    
    struct iso9660_directory_record* rec;
    rec = (struct iso9660_directory_record*)file->Reserved;

    if ((file->Offset + size) > rec->DataLength.Little) {
        size = rec->DataLength.Little - file->Offset;
    }

    uint64_t cnt = alignUp(size, 2048);
    size_t lbacount = __max(cnt / 2048, 1);
    for (size_t i = 0; i < lbacount; i++) {
        mount->Device->readSector(mount->Device, rec->ExtentLba.Little + i, 
                                  2048, &buffer[i * 2048]);
    }

    return size;
}

void isofsCloseFile(struct mounted_isofs* mount, struct fs_file* file)
{
    __unused(mount);
    struct iso9660_directory_record* dr = (struct iso9660_directory_record*)file->Reserved;
    mmAlignedFree(file->Reserved, dr->Size);
}

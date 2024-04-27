#include "vfs.h"
#include "config.h"

#include "memory/ring.h"
#include "mod/driver/driver.h"
#include "mod/driver/driver_kinterface.h"
#include "mod/driver/loader.h"
#include "mod/ld/elf_ld.h"

#include "memory/physical.h"
#include "partition.h"
#include "power/input.h"
#include "power/system.h"
#include "term/terminal.h"
#include "um/input.h"
#include "um/process.h"
#include "um/syscall.h"
#include "utils/string.h"
#include "utils/vector.h"

#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#define VFS_MAX_SCRATCH_SIZE 32768

static const char* letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789";
static struct vector mountPoints;
extern struct vector loadedDrivers;

static const char* analyzePath(const char* path, const char** premainder,
                               size_t* length, struct fs_mountpoint** mp)
{
    /* TODO IMPORTANT: Verify input properly */
    __unused(length);
    bool dynamicPath = false;
    struct process* proc = pcGetCurrentProcess();

    struct fs_mountpoint* chosenmp;
    const char* begin = path;
    const char* ptr = begin;

    while (*ptr) {
        if (*ptr == ':') {
            int dklen = ptr - begin;
            for (size_t i = 0; i < mountPoints.Length; i++) {
                struct fs_mountpoint* m = mountPoints.Data[i];
                if (!strncmp(begin, m->Letter, dklen)) {
                    chosenmp = m;
                    begin = ptr + 2;
                    break;
                }
            }
        }
        ptr++;
    }

    (*premainder) = begin;
    (*mp) = chosenmp;

    return dynamicPath ? path : NULL;
}

void vfsInit()
{
    mountPoints = vectorCreate(2);
}

size_t vfsGetPathComponent(const char* path)
{
    const char* ptr = path;
    const char* lastSlash = ptr;
    while (*ptr) {
        if (*ptr == '/') {
            lastSlash = ptr + 1;
        }

        ptr++;
    }

    return lastSlash - path;
}

struct fs_mountpoint* vfsCreateFilesystem(const char* filesystem,
                                          uint64_t beginLba,
                                          struct driver_disk_device_interface* device)
{
    static size_t letterIndex = 0;
    struct fs_mountpoint* mp = mmAllocKernelObject(struct fs_mountpoint);

    memset(mp, 0, sizeof(*mp));
    mp->Letter[0] = letters[letterIndex];
    letterIndex++;

    struct kdriver_manager* man = modGetFilesystemName(filesystem);
    struct driver_fs_interface* intf = man->Info->Interface;
    mp->Ops = intf->mount(intf, device, beginLba, mp);
    vectorInsert(&mountPoints, mp);

    mp->ScratchBuffer = mmAllocKernel(mp->BlockSize);
    mp->ScratchBufferLength = mp->BlockSize;
    return mp;
}

struct fs_file* vfsOpenFile(const char* path, int flags)
{
    const char* pcomponents;
    struct fs_mountpoint* mount;
    size_t dstrlen;
    const char* fr;
    if ((fr = analyzePath(path, &pcomponents, &dstrlen, &mount))) {
        mmAlignedFree(fr, dstrlen);
    }

    struct fs_file* file = mmAllocKernelObject(struct fs_file);
    file->Mounted = mount;
    file->Flags = flags;

    mount->Ops->createFile(mount->Ops, pcomponents, file);
    return file;
}

void vfsCloseFile(struct fs_file* file)
{
    file->Mounted->Ops->closeFile(file->Mounted->Ops, file);
    mmFreeKernelObject(file);
}

size_t vfsReadFile(struct fs_file* file, uint8_t* buffer, size_t size)
{
    struct fs_mountpoint* mp = file->Mounted;

    if (mp->ScratchBufferLength < size + file->Offset) {
        mmAlignedFree(mp->ScratchBuffer, mp->ScratchBufferLength);
        mp->ScratchBufferLength = size + file->Offset;

        mp->ScratchBufferLength = alignUp(mp->ScratchBufferLength, mp->BlockSize);
        mp->ScratchBuffer = mmAlignedAlloc(mp->ScratchBufferLength, 1);
    }

    size_t sz = mp->Ops->read(mp->Ops, mp->ScratchBuffer, size, file);
    memcpy(buffer, mp->ScratchBuffer + file->Offset, size);

    if (mp->ScratchBufferLength > VFS_MAX_SCRATCH_SIZE) {
        mmAlignedFree(mp->ScratchBuffer, mp->ScratchBufferLength);
        mp->ScratchBufferLength = VFS_MAX_SCRATCH_SIZE;
        mp->ScratchBuffer = mmAlignedAlloc(mp->ScratchBufferLength, 1);
    }

    return sz;
}

uint64_t syscOpenFile(union sysc_regs* regs)
{
    /* OpenFile( const char* file, int flags ) */
    if (!strncmp((const char*)regs->Arg1, "?STDOUT", 7)) {
        struct handle* h = pcCreateHandle(pcGetCurrentProcess(), HANDLE_TYPE_TTY, NULL);
        return h->Identifier;
    }

    struct fs_file* file = vfsOpenFile((const char*)regs->Arg1, regs->Arg2);
    if (file->Bad)
        return -1;

    struct handle* h = pcCreateHandle(pcGetCurrentProcess(), HANDLE_TYPE_FILE, file);
    return h->Identifier;
}

uint64_t syscReadFile(union sysc_regs* regs)
{
    /* ReadFile( int handle, char* buffer, size_t length ) */
    struct handle* buffer = pcGetHandleById(pcGetCurrentProcess(), regs->Arg1);
    switch (buffer->Type) {
    case HANDLE_TYPE_FILE: {
        if (!regs->Arg2 || !regs->Arg3)
            return -1;

        struct fs_file* file = buffer->Resource;
        return vfsReadFile(file, (uint8_t*)regs->Arg2, regs->Arg3);
    }
    case HANDLE_TYPE_PIPE: {
        struct pipe_data* pd = buffer->Resource;
        if (!regs->Arg2 || !regs->Arg3)
            return -1;

        if (pd->Flags & PIPE_FLAGS_INPUT_BUFFER) {
            struct input_event ev = inpReadEvent();
            if (regs->Arg3 != sizeof(struct input_event))
                return -1;
            memcpy((void*)regs->Arg2, &ev, sizeof(struct input_event));
            return sizeof(struct input_event);
        }
    }
    default:
        return -1;
    }
}

uint64_t syscWriteFile(union sysc_regs* regs)
{
    /* WriteFile( int handle, const char* buffer, size_t length ) */
    struct handle* buffer = pcGetHandleById(pcGetCurrentProcess(), regs->Arg1);
    switch (buffer->Type) {
    case HANDLE_TYPE_TTY:
        trmLog((const char*)regs->Arg2);
        break;
    case HANDLE_TYPE_PIPE: {
        struct pipe_data* pd = buffer->Resource;
        struct ringb* rb = pd->RingBuffer;
        if (pd->Flags & PIPE_FLAGS_READ_TO_TTY) {
            trmLog((const char*)regs->Arg2);
            break;
        }

        if (pd->Flags & PIPE_FLAGS_INPUT_BUFFER) {
            return -1;
        }

        struct pipe_packet* pp = mmAllocKernelObject(struct pipe_packet);
        pp->Data = mmAlignedAlloc(regs->Arg3, 1);
        pp->Length = regs->Arg3;

        memcpy(pp->Data, (void*)regs->Arg2, pp->Length);
        rbWrite(rb, pp);
        break;
    }
    default:
        return -1;
    }
    return regs->Arg3;
}

uint64_t syscSetFilePointer(union sysc_regs* regs)
{
    /* SetFilePointer( int fh, int rel, size_t offset ) */
    struct handle* buffer = pcGetHandleById(pcGetCurrentProcess(), regs->Arg1);
    switch (buffer->Type) {
    case HANDLE_TYPE_FILE: {
        struct fs_file* file = buffer->Resource;
        switch (regs->Arg2) {
        case SFILEPTR_REL_BEGIN:
            file->Offset = regs->Arg3;
            break;
        case SFILEPTR_REL_CURRENT:
            file->Offset += regs->Arg3;
            break;
        case SFILEPTR_REL_END:
            file->Offset = file->Size + regs->Arg3;
            break;
        default:
            return -1;
        }

        return file->Offset;
    }
    default:
        return -1;
    }

    return -1;
}

uint64_t syscMountVolume(union sysc_regs* regs)
{
    /* d0w */
    struct kdriver_manager* kd = modGetFilesystemName((const char*)regs->Arg2);
    struct driver_fs_interface* fs = kd->Info->Interface;
    if (!fs)
        return -1;

    struct vector* pa = partGetDisks();
    for (size_t i = 0; i < pa->Length; i++) {
        struct pt_disk* dk = pa->Data[i];
        for (size_t ii = 0; ii < dk->Partitions.Length; ii++) {
            struct partition* pt = dk->Partitions.Data[ii];
            if (uuidCompare(&pt->Identifier, (union ioctl_uuid*)regs->Arg3)) {
                if (dk->DeviceIndex != (int)regs->Arg1) {
                    return -1;
                }

                vfsCreateFilesystem(kd, pt->BeginLba, dk->Ops);
                return 0;
            }
        }
    }

    return -1;
}

#include "ioctl.h"
#include "partition.h"
#include "memory/physical.h"
#include "term/terminal.h"
#include "um/input.h"
#include "um/process.h"
#include <power/ioctl.h>

static uint64_t ioctlGetDisks(union sysc_regs* regs)
{
    if (regs->Arg4 != sizeof(struct ioctl_get_disk) || !regs->Arg3)
        return -1;
        
    struct vector* dks = partGetDisks();
    struct ioctl_get_disk* gd;
    gd = (struct ioctl_get_disk*)regs->Arg3;
    if (!gd->Data) {
        gd->Length = dks->Length;
        return 0;
    }

    for (size_t i = 0; i < gd->Length; i++) {
        struct pt_disk* p = dks->Data[i];
        gd->Data[i] = p->DeviceIndex;
    }

    return 0;
}

static struct partition* getPartition(struct pt_disk* dk, 
                                      union ioctl_uuid* uuid)
{
    for (size_t i = 0; i < dk->Partitions.Length; i++) {
        struct partition* pt = dk->Partitions.Data[i];
        if (uuidCompare(uuid, &pt->Identifier)) {
            return pt;
        }
    }

    return NULL;
}

static uint64_t ioctlSearchPartition(union sysc_regs* regs)
{
    if (regs->Arg4 != sizeof(struct ioctl_search_partition) || !regs->Arg3)
        return -1;

    struct ioctl_search_partition* se;
    se = (struct ioctl_search_partition*)regs->Arg3;

    struct vector* dks = partGetDisks();
    struct pt_disk* d;
    struct partition* pt;
    for (size_t i = 0; i < dks->Length; i++) {
        d = dks->Data[i];
        trmLogfn("finding.. %i", i);
        if (d->DeviceIndex == se->Disk) {
            pt = getPartition(d, &se->Identifier);
            trmLogfn("SearchPartition: found!");
            if (!pt)
                return -1;

            struct handle* h = pcCreateHandle(pcGetCurrentProcess(), HANDLE_TYPE_DEVICE, pt);
            return h->Identifier;
        }
    }

    return -1;
}

static uint64_t ioctlGetInputBuffer(union sysc_regs* regs)
{
    if (!inpAddReader()) {
        return -1;
    }

    struct pipe_data* pd = mmAllocKernelObject(struct pipe_data);
    pd->Flags = PIPE_FLAGS_INPUT_BUFFER;
    pd->RingBuffer = NULL;
    pd->ReferenceCount = 1;
    struct handle* h = pcCreateHandle(pcGetCurrentProcess(), HANDLE_TYPE_PIPE, pd);
    return h->Identifier;
}

uint64_t syscIoControl(union sysc_regs* regs)
{
    if (regs->Arg1)
        return -1;
    
    switch (regs->Arg2) {
    case IOCTL_GET_DISKS:
        return ioctlGetDisks(regs);
    case IOCTL_SEARCH_PARTITION:
        return ioctlSearchPartition(regs);
    case IOCTL_GET_INPUT_BUFFER:
        return ioctlGetInputBuffer(regs);
    }

    return -2;
}

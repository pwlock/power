#include "ring.h"
#include "memory/physical.h"
#include "term/terminal.h"

static 
int abs(int x)
{
    return x < 0 ? -x : x;
}

struct ringb* rbCreateBuffer(size_t length)
{
    struct ringb* rb = mmAllocKernelObject(struct ringb);
    rb->Length = length;
    rb->MemoryStart = mmAllocKernel(length * sizeof(void*));
    rb->ReadBegin = 0;
    rb->WriteBegin = 0;
    return rb;
}

void rbDestroy(struct ringb* rb)
{
    mmAlignedFree(rb->MemoryStart, sizeof(void*) * rb->Length);
    mmAlignedFree(rb, sizeof(struct ringb));
}

void rbWrite(struct ringb* rb, void* buffer)
{
    if ((rb->WriteBegin + 1) % rb->Length == 0) {
        trmLogfn("ring buffer packet lost: full");
        return;
    }

    rb->MemoryStart[rb->WriteBegin] = buffer;
    rb->WriteBegin = (rb->WriteBegin + 1) % rb->Length;
}

void* rbRead(struct ringb* rb)
{
    if (rb->WriteBegin == rb->ReadBegin) {
        return NULL;
    }

    void* value = rb->MemoryStart[rb->ReadBegin];
    rb->ReadBegin = (rb->ReadBegin + 1) % rb->Length;
    return value;
}

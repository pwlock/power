#include "ring_buffer.h"
#include "memory/physical.h"

struct ring
{
    void* Data; /* Length = ObjectSize * ObjectCount */
    size_t ObjectSize;
    int ObjectCount;

    int WriteEnd;
    int ReadEnd;
};

struct ring* ringInit(size_t objSize, int objCount)
{
    struct ring* r = mmAllocKernelObject(struct ring);
    memset(r, 0, sizeof(*r));

    r->ObjectSize = objSize;
    r->ObjectCount = objCount;
    r->Data = mmAllocKernel(objSize * objCount);
    return r;
}

void ringDestroy(struct ring* ring)
{
    mmAlignedFree(ring->Data, ring->ObjectCount * ring->ObjectSize);
    mmFreeKernelObject(ring);
}

bool ringWrite(struct ring* ring, const void* object)
{
    if ((ring->WriteEnd + ring->ObjectSize) % ringGetTotalCapacity(ring) == 0)
        return false;

    memcpy((ring->Data + ring->WriteEnd), object, ring->ObjectSize);
    ring->WriteEnd = (ring->WriteEnd + ring->ObjectSize) % ringGetTotalCapacity(ring);
    return true;
}

bool ringRead(struct ring* ring, void* outObj)
{
    if (ring->ReadEnd == ring->WriteEnd)
        return false;

    memcpy(outObj, (ring->Data + ring->ReadEnd), ring->ObjectSize);
    ring->ReadEnd = (ring->ReadEnd + ring->ObjectSize) % ringGetTotalCapacity(ring);
    return true;
}

size_t ringGetObjectSize(struct ring* ring)
{
    return ring->ObjectSize;
}

size_t ringGetTotalCapacity(struct ring* ring)
{
    return ring->ObjectSize * ring->ObjectCount;
}

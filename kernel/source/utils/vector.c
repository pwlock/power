#include "vector.h"
#include "memory/physical.h"
#include "term/terminal.h"

#define GROW_SCALE ((sizeof(void*) * 4))
#define GROW(X) (X * GROW_SCALE)

static void** resizeBuffer(void** oldBuffer, size_t* length)
{
    size_t oldLength = *length;
    void** newb = mmAllocKernel((oldLength * GROW_SCALE) + GROW_SCALE);
    memset(newb, 0, GROW(oldLength) + GROW_SCALE);
    memcpy(newb, oldBuffer, oldLength * sizeof(void*));
    (*length) = oldLength + 4;

    if (oldBuffer) {
        mmAlignedFree(oldBuffer, oldLength * sizeof(void*));
    }

    return newb;
}

struct vector vectorCreate(size_t initialLength)
{
    struct vector v = {};
    v.Capacity = initialLength;
    v.Length = 0;
    v.Data = NULL;

    if (initialLength != 0) {
        v.Data = mmAllocKernel(sizeof(void*) * initialLength);
    }

    return v;
}

void vectorInsert(struct vector* v, void* data)
{
    if (v->Length >= v->Capacity) {
        v->Data = resizeBuffer(v->Data, &v->Capacity);
    }

    v->Data[v->Length] = data;
    v->Length++;
}

void* vectorRemove(struct vector* v, void* data)
{
    bool seen = false;
    void** d, **result;

    for (size_t i = 0; i < v->Length; i++) {
        d = &v->Data[i];
        result = &v->Data[i + 1];

        if (*d == data) {
            seen = true;
        }

        if (seen && i < v->Length - 1) {
            (*d) = (*result); 
        }
    }

    v->Length--;
    return data;
}

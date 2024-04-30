#include "vector.h"
#include "memory/physical.h"

static void** resizeBuffer(void** oldBuffer, size_t* length)
{
    void** newb = mmAlignedAlloc(*length + (sizeof(void*) * 4), 1);
    memset(newb, 0, *length + (sizeof(void*) * 4));
    memcpy(newb, oldBuffer, *length * sizeof(void*));
    (*length) = *length + 4;

    return newb;
}

struct vector vectorCreate(size_t initialLength)
{
    struct vector v = {};
    v.Capacity = initialLength;
    v.Length = 0;
    v.Data = NULL;

    if (initialLength != 0) {
        v.Data = mmAlignedAlloc(sizeof(void*) * initialLength, 1);
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

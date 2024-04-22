/* vector.h
   The implementation of a vector */
#pragma once

#include <stddef.h>

struct vector
{
    size_t Length;
    size_t Capacity;
    void** Data;
};

struct vector vectorCreate(size_t initalLength);
void vectorInsert(struct vector*, void* data);
void vectorMerge(struct vector* mergee, const struct vector* merged);
void* vectorRemove(struct vector*, void* data);

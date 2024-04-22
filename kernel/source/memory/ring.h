/* ring.h
   Purpose: The implementation of a ring buffer */
#pragma once

#include "config.h"

struct ringb
{
    void** MemoryStart;
    size_t Length;
    int WriteBegin;
    int ReadBegin;
};

struct ringb* rbCreateBuffer(size_t length);
void rbDestroy(struct ringb*);
void rbWrite(struct ringb*, void* buffer);
void* rbRead(struct ringb*);

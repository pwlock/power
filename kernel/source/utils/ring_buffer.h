/* ring_buffer.h
   Purpose: A ring buffer. */
#pragma once

#include "config.h"

struct ring* ringInit(size_t objSize, int objCount);
void ringDestroy(struct ring* ring);

bool ringWrite(struct ring*, const void* object);
bool ringRead(struct ring*, void* outObject);

size_t ringGetObjectSize(struct ring*);
size_t ringGetTotalCapacity(struct ring*);

/* timer_interface.h
   Purpose: Generic internal time interface */
#pragma once

#include <stddef.h>
#include <stdint.h>

typedef void*(*timer_create_pfn_t)();

struct timer {
    void (*setReloadTime)(struct timer*, size_t units);
    int (*getType)(struct timer*);
    uint64_t (*getMillisecondUnit)(struct timer*);
    void* Reserved[1];

    int EnabledFlag;
};

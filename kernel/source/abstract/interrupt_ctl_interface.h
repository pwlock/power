/* interrupt_ctl_interface.h
   Purpose: Generic interface for interrupt controllers */
#pragma once

#include <stdint.h>
typedef void(*int_handler_pfn_t)(void* data);

struct int_ctl
{
    int (*getControllerType)(struct int_ctl*);
    void (*ackInterrupt)(struct int_ctl*);
    void (*handleInterrupt)(struct int_ctl*, int vector, 
                            int_handler_pfn_t ih, void* data);
    void (*executeInterrupt)(struct int_ctl*, int vector);

    /* Only really used when in PIC. */
    int (*isVectorFree)(struct int_ctl*, int vector);
    
    uint64_t (*getMsiAddress)(struct int_ctl*, uint64_t* data, int vector);
};

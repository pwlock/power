#include "interrupt_ctl.h"
#include "arch/i386/int/apic.h"
#include <stddef.h>

#define ARRAY_LENGTH(A) sizeof(*A) / sizeof(A)

typedef void*(*intctl_create_pfn_t)();

static struct int_ctl* dftInstance = NULL;

struct int_ctl* intCtlCreateDefault()
{
    void* ret;
    static intctl_create_pfn_t creators[] = { (intctl_create_pfn_t)apicCtlCreate };
    for (size_t i = 0; i < ARRAY_LENGTH(creators); i++) {
        if ((ret = creators[i]()))
            break; 
    }

    dftInstance = ret;
    return ret;
}

struct int_ctl* intCtlGetDefault()
{
    return dftInstance;
}

int intCtlGetControllerType()
{
    return dftInstance->getControllerType(dftInstance);
}

void intCtlAckInterrupt()
{
    return dftInstance->ackInterrupt(dftInstance);
}

void intCtlHandleInterrupt(int vector, int_handler_pfn_t ih, void* data)
{
    return dftInstance->handleInterrupt(dftInstance, vector, ih, data);
}

void intCtlExecuteInterrupt(int vector)
{
    return dftInstance->executeInterrupt(dftInstance, vector);
}

uint64_t intCtlGetMsiAddress(uint64_t* data, int vector)
{
    return dftInstance->getMsiAddress(dftInstance, data, vector);
}

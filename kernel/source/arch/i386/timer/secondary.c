/* secondary.c
   Purpose: creation of secondary timer objects */

#include "abstract/timer.h"
#include "abstract/timer_interface.h"
#include "apict.h"

static struct timer* dflInstance = NULL;

void tmCreateSecondaryTimer()
{
    void* ret;
    static timer_create_pfn_t creators[] = { (timer_create_pfn_t)apicCreateTimer };

    if (tmGetType(tmGetDefaultTimer()) == TIMER_TYPE_PIT) {
        /* Use the PIT in periodic as the sec timer. */
        dflInstance = tmGetDefaultTimer();
    }

    for (size_t i = 0; i < ARRAY_LENGTH(creators); i++) {
        if ((ret = creators[i]()))
            break;
    }
}

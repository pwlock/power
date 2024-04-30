#include "synchronization.h"
#include "memory/physical.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread.h"
#include "term/terminal.h"
#include "utils/vector.h"

#define THREAD_MAX 10

struct s_event
{
    int Threads[THREAD_MAX];
    uint8_t ArrayIndex;
    bool Flag;
};

struct s_event* schedCreateEvent()
{
    struct s_event* event = mmAllocKernelObject(struct s_event*);
    memset(event, 0, sizeof(struct s_event));
    return event;
}

void schedDestroyEvent(struct s_event* event)
{
    schedEventRaise(event);
    mmFreeKernelObject(event);
}

void schedEventRaise(struct s_event* event)
{
    if (event->Flag)
        return;

    struct thread* t;
    for (int i = 0; i < event->ArrayIndex; i++) {
        t = schedGetThreadById(event->Threads[i]);
        if (!t)
            continue;
        t->Suspended = false;
    }

    event->Flag = true;
}

void schedEventPause(struct s_event* event) 
{
    if (event->Flag)
        return;

    schedEnable(false);
    struct thread* t = schedGetCurrentThread();
    t->Suspended = true;

    event->Threads[event->ArrayIndex] = t->Identifier;
    event->ArrayIndex++;
    schedThreadYield(t);
    schedEnable(true);

    asm volatile("sti; hlt");
}

bool schedEventFlag(struct s_event* event)
{
    return event->Flag;
}

void schedEventResetFlag(struct s_event* event)
{
    event->ArrayIndex = 0;
    event->Flag = false;
}

/* mutex */
struct s_mutex
{
    volatile int Locker; /*< The thread that currently has the lock */
};

struct s_mutex* schedCreateMutex()
{
    struct s_mutex* mtx = mmAllocKernelObject(struct s_mutex);
    mtx->Locker = 0;
    return mtx;
}

void schedDestroyMutex(struct s_mutex* mtx)
{
    mtx->Locker = 0;
    mmFreeKernelObject(mtx);
}

void schedMutexAcquire(struct s_mutex* mtx)
{
    struct thread* t = schedGetCurrentThread();
    while (!__sync_bool_compare_and_swap(&mtx->Locker, 0, t->Identifier));
}

void schedMutexRelease(struct s_mutex* mtx)
{
    mtx->Locker = 0;
}

/* semaphore */
struct s_semaphore 
{
    int Slots;
    struct vector Threads;
};

struct s_semaphore* schedCreateSemaphore(int slots)
{
    struct s_semaphore* s = mmAllocKernelObject(struct s_semaphore);
    memset(s, 0, sizeof(struct s_semaphore));

    s->Threads = vectorCreate(1);
    s->Slots = slots;
    return s;
}

void schedSemaphoreAcquire(struct s_semaphore* s)
{
    if (!s->Slots) {
        struct thread* t = schedGetCurrentThread();
        t->Suspended = true;

        vectorInsert(&s->Threads, t);
        asm volatile("hlt");
    }

    s->Slots--;
}

void schedSemaphoreRelease(struct s_semaphore* s) 
{
    s->Slots++;
    if (s->Threads.Length) {
        struct thread* t = s->Threads.Data[0];
        t->Suspended = false;
        
        vectorRemove(&s->Threads, t);
    }
}

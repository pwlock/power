#include "scheduler.h"
#include "abstract/interrupt_ctl.h"
#include "arch/i386/fpstate.h"
#include "arch/i386/gdt.h"
#include "memory/physical.h"
#include "scheduler/thread.h"
#include "term/terminal.h"
#include "utils/vector.h"

struct thread_list {
    struct thread_list* Previous;
    struct thread_list* Next;

    struct thread* Thread;
};

extern void schedTaskSwitch(struct thread_registers* registers);
extern void schedIntersegmentTaskSwitch(struct thread_registers* registers);

struct scheduler {
    struct thread_list* ExecList; /*< List of threads currently for scheduling */
    struct thread_list* CurrentExec;
    struct vector SleepingThreads;
    int ExecCount;
    bool ShouldSaveRegisters;
};

static struct scheduler* defScheduler = NULL;
static bool dsEnabled = false;

static inline uint64_t readTimestamp(void)
{
    uint32_t low, high;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

static void linkedListAppend(struct thread_list** head,
                             struct thread_list* item)
{
    if (!*head) {
        (*head) = item;
        return;
    }

    struct thread_list* tmp = *head;
    while (tmp->Next != NULL) {
        tmp = tmp->Next;
    }

    tmp->Next = item;
    item->Previous = tmp;
}

static inline bool
updateThreadTimings(struct thread* thread)
{
    uint64_t currentct = readTimestamp();
    thread->Timing.ElapsedCount += currentct - thread->Timing.LastCount;
    thread->Timing.LastCount = currentct;

    thread->Timing.CurrentQuantum--;
    if (!thread->Timing.CurrentQuantum) {
        thread->Timing.CurrentQuantum = thread->Timing.ResetQuantum;
        return true;
    }

    return false;
}

/* Optionally put the sleeping threads to wake */
static inline void unsuspendThreads()
{
    for (size_t i = 0; i < defScheduler->SleepingThreads.Length; i++) {
        struct thread* thr = defScheduler->SleepingThreads.Data[i];
        if (thr->SuspendedTicks != THREAD_NOT_SLEEPING) {
            if (--thr->SuspendedTicks) {
                /* Events may wake the thread up. */
                if (!thr->Suspended)
                    thr->Suspended = true;
                continue;
            }

            thr->SuspendedTicks = THREAD_NOT_SLEEPING;
            thr->Suspended = false;
            vectorRemove(&defScheduler->SleepingThreads, thr);
        }
    }
}

static inline void
nextThread()
{
    do {
        defScheduler->CurrentExec = defScheduler->CurrentExec->Next;
        if (!defScheduler->CurrentExec) {
            defScheduler->CurrentExec = defScheduler->ExecList;
            break;
        }
    } while (defScheduler->CurrentExec != NULL
             && defScheduler->CurrentExec->Thread->Suspended);
}

static void idleTask()
{
    while (true) {
        schedThreadYield(schedGetCurrentThread());
        asm volatile("sti; hlt");
    }
}

static inline void loadRegisters(const struct idt_register_state* state)
{
    if (defScheduler->CurrentExec->Thread->Timing.LastCount) {
        struct thread_registers regs;
        regs.GeneralRegisters = *state->GeneralRegisters;
        regs.Pointers = *state->PointerRegisters;

        defScheduler->CurrentExec->Thread->Registers.CodeSegment = state->PointerRegisters->Cs;
        uint64_t cs = defScheduler->CurrentExec->Thread->Registers.CodeSegment;
        if (!(cs & 0b11)) {
            defScheduler->CurrentExec->Thread->Registers.DataSegment &= ~3;
        } else {
            defScheduler->CurrentExec->Thread->Registers.DataSegment |= 3;
        }

        fpsSave(defScheduler->CurrentExec->Thread->Registers.FloatingPointState);
        memcpy(&defScheduler->CurrentExec->Thread->Registers.GeneralRegisters,
               state->GeneralRegisters, sizeof(*state->GeneralRegisters));
        memcpy(&defScheduler->CurrentExec->Thread->Registers.Pointers,
               state->PointerRegisters, sizeof(*state->PointerRegisters));
    }
}

struct scheduler* schedCreate()
{
    defScheduler = mmAllocKernelObject(struct scheduler);
    memset(defScheduler, 0, sizeof(*defScheduler));

    defScheduler->ShouldSaveRegisters = true;
    defScheduler->SleepingThreads = vectorCreate(3);
    schedAddThread(schedCreateThread(idleTask, NULL, 0));
    return defScheduler;
}

bool schedIsEnabled()
{
    return dsEnabled;
}

void schedEnable(bool flag)
{
    dsEnabled = flag;
}

void schedAddThread(struct thread* thread)
{
    struct thread_list* ls = mmAllocKernelObject(struct thread_list);
    ls->Next = NULL;
    ls->Previous = NULL;
    ls->Thread = thread;

    defScheduler->ExecCount++;
    ls->Thread->Identifier = defScheduler->ExecCount;
    linkedListAppend(&defScheduler->ExecList, ls);
}

void schedRemoveThread(const struct thread* t)
{
    struct thread_list* ls = defScheduler->ExecList;
    while (ls->Next != NULL) {
        if (ls->Thread == t)
            break;
        ls = ls->Next;
    }

    if (t == defScheduler->CurrentExec->Thread) {
        defScheduler->CurrentExec = defScheduler->CurrentExec->Next;
        defScheduler->ShouldSaveRegisters = false;
    }

    if (!ls->Next && !ls->Previous) {
        /* Is the head and there is not another thread. */
        defScheduler->ExecList = NULL;
    } else {
        if (ls->Previous)
            ls->Previous->Next = ls->Next;
        if (ls->Next)
            ls->Next->Previous = ls->Previous;
    }

    schedFreeThread((struct thread*)t);
}

void schedThink(const struct idt_register_state* state)
{
    if (!defScheduler->CurrentExec) {
        defScheduler->CurrentExec = defScheduler->ExecList;
    }

    /* Redundant check but necessary if ExecList is NULL. */
    if (!defScheduler->ExecCount
        || !defScheduler->CurrentExec)
        return;

    unsuspendThreads();
    if (defScheduler->ShouldSaveRegisters) {
        loadRegisters(state);
    }
    defScheduler->ShouldSaveRegisters = true;

    if (updateThreadTimings(defScheduler->CurrentExec->Thread)
        || defScheduler->CurrentExec->Thread->Suspended) {
        nextThread();
        if (!defScheduler->CurrentExec)
            return;
    }

    if (!defScheduler->CurrentExec->Thread->Timing.LastCount) {
        defScheduler->CurrentExec->Thread->Timing.LastCount = readTimestamp();
    }

    intCtlAckInterrupt();
    if (defScheduler->CurrentExec->Thread->Registers.CodeSegment != KERNEL_CODE_SEGMENT) {
        schedIntersegmentTaskSwitch(&defScheduler->CurrentExec->Thread->Registers);
    } else
        schedTaskSwitch(&defScheduler->CurrentExec->Thread->Registers);
}

struct thread* schedGetCurrentThread()
{
    if (!defScheduler->CurrentExec)
        return NULL;
    return defScheduler->CurrentExec->Thread;
}

struct thread* schedGetThreadById(int id)
{
    struct thread_list* tmp = defScheduler->ExecList;
    while (tmp != NULL) {
        if (tmp->Thread->Identifier == id)
            return tmp->Thread;
        tmp = tmp->Next;
    }

    return NULL;
}

void schedSleep(int ticks)
{
    struct thread* thr = schedGetCurrentThread();
    thr->Suspended = true;
    thr->SuspendedTicks = ticks;
    
    vectorInsert(&defScheduler->SleepingThreads, thr);
    asm volatile("sti; hlt");
}

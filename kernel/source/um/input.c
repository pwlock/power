#include "input.h"
#include "memory/physical.h"
#include "memory/ring.h"
#include "power/input.h"
#include "scheduler/synchronization.h"
#include "term/terminal.h"

struct {
    struct s_event* ReaderEvent;
    struct ringb* InputRing;
    int ReaderCount;
} input;

void inpCreateInputRing()
{
    input.ReaderEvent = schedCreateEvent();
    input.InputRing = rbCreateBuffer(24);
}

bool inpAddReader()
{
    if (input.ReaderCount)
        return false;
    input.ReaderCount++;
    return true;
}

void inpRemoveReader()
{
    schedEventResetFlag(input.ReaderEvent);
    input.ReaderCount = 0;
}

struct input_event* inpReadEvent()
{
    struct input_event* ev = rbRead(input.InputRing);
    if (!ev) {
        schedEventPause(input.ReaderEvent);
        schedEventResetFlag(input.ReaderEvent);
        ev = rbRead(input.InputRing);
    }

    return ev;
}

void inpSendInputEvent(int type, int value, int subFlags)
{
    if (!input.ReaderCount) {
        trmLogfn("not writing event %p: no one to read!", type);
        return;
    }

    struct input_event* ev = mmAllocKernelObject(struct input_event);
    ev->Code = value;
    ev->Type = type;
    ev->Flags = subFlags;
    ev->Reserved = 0;

    rbWrite(input.InputRing, ev);
    schedEventRaise(input.ReaderEvent);
}

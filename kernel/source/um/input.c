#include "input.h"
#include "memory/physical.h"
#include "power/input.h"
#include "scheduler/synchronization.h"
#include "term/terminal.h"
#include "utils/ring_buffer.h"

struct {
    struct ring* InputRing;
    struct s_event* ReaderEvent;
    int ReaderCount;
} input;

void inpCreateInputRing()
{
    input.InputRing = ringInit(sizeof(struct input_event), 24);
    input.ReaderEvent = schedCreateEvent();
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

struct input_event inpReadEvent()
{
    struct input_event event;
    bool ev = ringRead(input.InputRing, &event);
    if (!ev) {
        schedEventPause(input.ReaderEvent);
        schedEventResetFlag(input.ReaderEvent);
        ev = ringRead(input.InputRing, &event);
    }

    return event;
}

void inpSendInputEvent(int type, int value, int subFlags)
{
    if (!input.ReaderCount) {
        trmLogfn("not writing event %p: no one to read!", type);
        return;
    }

    struct input_event ev;
    ev.Code = value;
    ev.Type = type;
    ev.Flags = subFlags;
    ev.Reserved = 0;

    ringWrite(input.InputRing, &ev);
    schedEventRaise(input.ReaderEvent);
}

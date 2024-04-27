#include "process.h"
#include "arch/i386/paging.h"
#include "memory/address.h"
#include "memory/physical.h"
#include "mod/elf.h"
#include "mod/ld/elf_ld.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread.h"
#include "term/terminal.h"
#include "utils/ring_buffer.h"
#include "utils/vector.h"
#include "utils/string.h"
#include "vfs.h"

struct process_manager
{
    struct process* Processes;
};

static struct process_manager pman;

static void* mapString(address_space_t* as, const char* str) 
{
    if (!str) {
        return NULL;
    }

    size_t strl = strlen(str) + 1;
    strl = alignUp(strl, 4096);
    
    struct address_space_mgr* mgr = addrGetManagerForCr3(as);
    uint64_t offset = asmgrGetDataPage(mgr, strl);
    for (size_t i = 0; i < strl; i += 4096)
        pgMapPage(as, (uint64_t)str + i, offset + i, PT_FLAG_USER);

    return (void*)offset;
}

static void createStreams(struct process* p)
{
    if (p->Parent) {
        memcpy(p->Streams, p->Parent->Streams, sizeof(p->Streams));

        for (int i = 0; i < 3; i++) {
            struct handle* h = mmAllocKernelObject(struct handle);
            struct handle* parenth = p->Parent->Handles.Data[i];

            h->Identifier = i;
            h->Type = HANDLE_TYPE_PIPE;
            h->Resource = parenth->Resource;
            ((struct pipe_data*)h->Resource)->ReferenceCount++;
            vectorInsert(&p->Handles, h);
        }
        
        return;
    }

    for (int i = 0; i < 3; i++) {
        struct handle* h = mmAllocKernelObject(struct handle);
        h->Resource = mmAllocKernelObject(struct pipe_data);
        ((struct pipe_data*)h->Resource)->RingBuffer = ringInit(sizeof(void*), 12);
        ((struct pipe_data*)h->Resource)->Flags = PIPE_FLAGS_READ_TO_TTY;
        
        h->Identifier = i;
        h->Type = HANDLE_TYPE_PIPE;
        vectorInsert(&p->Handles, h);
    }
}

void pcProcessManagerInit()
{
    pman.Processes = NULL;
}

struct process* pcCreateProcess(struct process* parent, const char* name,
                                const char* cmdargs)
{
    struct process* p = mmAllocKernelObject(struct process);
    memset(p, 0, sizeof(struct process));

    p->Children = vectorCreate(1);
    p->Threads = vectorCreate(1);
    p->Handles = vectorCreate(3);

    if (!parent) {
        pman.Processes = p;
        size_t length = vfsGetPathComponent(name);
        p->WorkingDirectory = mmAllocKernel(length + 1);
        memset(p->WorkingDirectory, 0, length + 1);
        memcpy(p->WorkingDirectory, name, length);
    } 
    else {
        p->WorkingDirectory = parent->WorkingDirectory;
        vectorInsert(&parent->Children, p);
    }

    struct fs_file* f = vfsOpenFile(name, 0);
    uint8_t* exe = mmAllocKernel(f->Size);
    vfsReadFile(f, exe, f->Size);

    struct elf_executable ex = modLoadElf(exe, 
                ELF_LOAD_USER_ADDRESS_SPACE | ELF_LOAD_NEW_ADDRESS_SPACE);
    p->CommandLine = mapString(ex.AddressSpace, cmdargs);

    struct thread_args args = { ex.AddressSpace, 0 };
    struct thread* t = schedCreateThreadEx((thread_execution_pfn_t)((uint8_t*)ex.EntryPoint), 
                                &args, THREAD_CREATE_USER_MODE);
    t->Process = p;
    p->SyscallStack = threadAllocateStack(ex.AddressSpace, 4096);
    createStreams(p);

    schedAddThread(t);
    vectorInsert(&p->Threads, t);

    mmAlignedFree(exe, f->Size);
    vfsCloseFile(f);
    schedThreadYield(schedGetCurrentThread());

    return p;
}

struct process* pcGetCurrentProcess() 
{
    return schedGetCurrentThread()->Process;
}

struct handle* pcCreateTerminalHandle(struct process* process)
{
    struct handle* ha = mmAllocKernelObject(struct handle);
    ha->Identifier = process->Handles.Length;
    ha->Type = HANDLE_TYPE_TTY;
    ha->Resource = NULL;

    vectorInsert(&process->Handles, ha);
    return ha;
}

void pcCloseHandle(struct process* p, struct handle* handle)
{
    switch(handle->Type) { 
    case HANDLE_TYPE_FILE:
        vfsCloseFile(handle->Resource);
    case HANDLE_TYPE_TTY:
    default:
        break;
    }

    vectorRemove(&p->Handles, handle);
    mmFreeKernelObject(handle);
}

void pcTerminateProcess(struct process* p) {
    size_t i;
    for (i = 0; i < p->Threads.Length; i++) {
        schedRemoveThread(p->Threads.Data[i]);
    }

    for (i = 0; i < p->Handles.Length; i++) {
        struct handle* h = p->Handles.Data[i];
        pcCloseHandle(p, h);
        mmFreeKernelObject(h);
    }

    if (p->Parent) {
        vectorRemove(&p->Parent->Children, p);
    }
    else {
        pman.Processes = NULL;
    }

    mmFreeKernelObject(p);
}

struct handle* pcGetHandleById(struct process* process , int handle)
{
    for (size_t i = 0; i < process->Handles.Length; i++) {
        struct handle* ha = process->Handles.Data[i];
        if (ha->Identifier == handle) {
            return ha;
        }
    }

    return NULL;
}

struct handle*
pcCreateHandle(struct process* process, int type, void* res)
{
    struct handle* ha = mmAllocKernelObject(struct handle);
    ha->Identifier = process->Handles.Length;
    ha->Type = type;
    ha->Resource = res;

    vectorInsert(&process->Handles, ha);
    return ha;
}

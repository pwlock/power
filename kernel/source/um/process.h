/* process.h
   Purpose: Process manager */
#pragma once

#include "memory/ring.h"
#include "utils/vector.h"
#include "vfs.h"
#include <stddef.h>

#define HANDLE_TYPE_TTY 1
#define HANDLE_TYPE_FILE 2
#define HANDLE_TYPE_DEVICE 3
#define HANDLE_TYPE_PIPE 4

#define STREAM_STDOUT 0
#define STREAM_STDIN 2
#define STREAM_STDERR 1

#define PIPE_FLAGS_READ_TO_TTY (1 << 0) /*< When writing, immediately read and place onto the terminal*/
#define PIPE_FLAGS_INPUT_BUFFER (1 << 1) /*< Direct IO calls to input buffer calls. */

struct handle
{
    int Type;
    int Identifier;
    void* Resource;
};

struct pipe_data
{
    struct ringb* RingBuffer;
    int ReferenceCount;
    int Flags;
};

struct pipe_packet
{
    size_t Length;
    const char* Data;
};

struct process
{
    struct process* Parent;
    void* SyscallStack; /*< Stack used for syscalls */
    
    struct handle Streams[3]; 
    struct vector Children;
    struct vector Threads;
    struct vector Handles;
    char* CommandLine;
    char* WorkingDirectory;
};

void pcProcessManagerInit();
struct process* pcCreateProcess(struct process* parent, 
                                const char* name, const char* args);
struct handle* pcCreateHandle(struct process*, int type, void* res);
struct handle* pcGetHandleById(struct process*, int handle);
void pcCloseHandle(struct process*, struct handle* handle);
struct process* pcGetCurrentProcess();

void pcTerminateProcess(struct process*);


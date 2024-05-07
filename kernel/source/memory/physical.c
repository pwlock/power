#include "physical.h"
#include "arch/i386/paging.h"
#include "bootloader_requests.h"
#include "limine.h"
#include "term/terminal.h"
#include <stdint.h>

#define min(X, y) ((X) > (y) ? y : X)
#define GET_DATA_PTR(B) (uint64_t)((uint8_t*)B) - (B->Size)
#define GET_HEADER_PTR(B, l) PaAdd(B, l)

typedef struct __attribute__((packed)) 
mm_block
{
    struct mm_block* Next;
    uint16_t Alignment;
    size_t Size;
} mm_block;

typedef struct __attribute__((packed)) 
mm_global
{
    mm_block* FreeList;
    void* Head;
    size_t BumpLength;
} mm_global;

static mm_global* globalMemory = NULL;

static void appendLinkedList(mm_block** head, mm_block* block)
{
    if (!*head) {
        (*head) = block;
        return;
    }

    mm_block* tmp = *head;
    while(tmp->Next != NULL) {
        tmp = tmp->Next;
    }

    tmp->Next = block;
}

static void removeLinkedList(mm_block* head, mm_block* block)
{
    if (!head) {
        trmLogfn("removeLinkedList: head is NULL");
        return;
    }

    mm_block* tmp = head;
    while (tmp->Next != block) {
        if (!tmp) {
            return;
        }
        tmp = tmp->Next;
    }

    tmp->Next = block->Next;
}

static void* linkedList(size_t size, uint16_t alignment)
{
    mm_block* tmp = globalMemory->FreeList;
    mm_block* chosen = NULL;
    while (tmp->Next != NULL) {
        if (tmp->Size >= size && tmp->Alignment % alignment == 0) {
            chosen = tmp;
            break;
        }
    }

    if (chosen == NULL)
        return NULL;

    if (chosen->Size > size
     && chosen->Size - size >= sizeof(mm_block) + 1) {
        uint64_t dataptr = GET_DATA_PTR(chosen);
        mm_block* newb = (mm_block*)(dataptr + size + sizeof(mm_block));

        newb->Size = size;
        newb->Alignment = 1;
        newb->Next = NULL;
        appendLinkedList(&globalMemory->FreeList, newb);

        chosen->Size -= size - sizeof(mm_block);
        chosen = newb;
    }

    removeLinkedList(globalMemory->FreeList, chosen);
    return (void*)GET_DATA_PTR(chosen);
}

void mmInit(void* base, size_t length)
{
    globalMemory = base;
    globalMemory->FreeList = NULL;
    globalMemory->Head = (void*)((uint64_t)base + sizeof(*globalMemory));
    globalMemory->BumpLength = length - sizeof(mm_global);
}

void mmAddUsablePart(void* begin, size_t length)
{
    if (length == 0) return;
    
    struct mm_block* blk = PaAdd(begin, length - sizeof(mm_block));

    blk->Next = NULL;
    blk->Alignment = 1;
    blk->Size = length - sizeof(struct mm_block);

    appendLinkedList(&globalMemory->FreeList, blk);
}

void* mmAlignedAlloc(size_t size, uint16_t alignment)
{
    if (alignment == 0) 
        alignment = 1;

    uint64_t head = (uint64_t)globalMemory->Head;
    uint64_t overhead = alignment - head % alignment;

    head += overhead;
    if (size + overhead > globalMemory->BumpLength) {
        /* No space to fit aligned block in bump.
           Use linked list then. */
        return linkedList(size, alignment);
    }

    head += size;
    mm_block* newb = (mm_block*)head;
    newb->Size = size;
    newb->Alignment = alignment;
    newb->Next = NULL;
    
    globalMemory->Head = (void*)head + sizeof(mm_block);
    return (void*)GET_DATA_PTR(newb);
}

void mmAlignedFree(const void* ptr, size_t size)
{
    if (ptr == NULL) return;

    mm_block* header = GET_HEADER_PTR(ptr, size);
    appendLinkedList(&globalMemory->FreeList, header);
}

void* mmGetGlobal()
{
    return globalMemory;
}

size_t mmGetLength()
{
    return globalMemory->BumpLength;
}

bool mmIsPhysical(void* ptr)
{
    uint64_t cast = (uint64_t)ptr;
    struct limine_memmap_response* mm = rqGetMemoryMapRequest();

    for (uint64_t i = 0; i < mm->entry_count; i++) {
        struct limine_memmap_entry* ent = mm->entries[i];
        if (ent->type == LIMINE_MEMMAP_USABLE
         && (ent->base <= cast && (ent->base + ent->length) >= cast)) {
            return true;
        }
    }

    return false;
}

void* mmAllocKernel(size_t size)
{
    void* ptr = mmAlignedAlloc(size, 1);
    memset(ptr, 0, size);
    return ptr;
}

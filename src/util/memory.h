
#ifndef _MEMORY_H
#define _MEMORY_H

#include <stddef.h>

// Aligns size to 8 bytes
#define ALIGN_8(size) (((size) + 0x7) & ~0x7)

extern struct HeapSegment* gFirstHeapSegment;

#define MALLOC_FREE_BLOCK 0xFEEE
#define MALLOC_USED_BLOCK 0xEEEF

#define MALLOC_BLOCK_HEAD 0xEEAD0000
#define MALLOC_BLOCK_FOOT 0xF0000000

struct HeapUsedSegment
{
    unsigned int header;
    void* segmentEnd;
};

struct HeapSegment
{
    unsigned int header;
    void* segmentEnd;
    struct HeapSegment* nextSegment;
    struct HeapSegment* prevSegment;
};

struct HeapSegmentFooter
{
    struct HeapSegment* header;
    unsigned int footer;
};

#define MIN_HEAP_BLOCK_SIZE (sizeof(struct HeapSegment) + sizeof(struct HeapSegmentFooter))

void heapInit(void* heapStart, void* heapEnd);
void heapReset();
void *cacheFreePointer(void* target);
void *malloc(unsigned int size);
void *realloc(void* target, unsigned int size);
void free(void* target);
int calculateBytesFree();
int calculateHeapSize();
int calculateLargestFreeChunk();
extern void zeroMemory(void* memory, int size);
extern void memCopy(void* target, const void* src, int size);

#endif
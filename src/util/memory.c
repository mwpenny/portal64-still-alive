#include "memory.h"

struct HeapSegment* gFirstFreeSegment;
void* gHeapStart;
void* gHeapEnd;


void heapInitBlock(struct HeapSegment* segment, void* end, int type)
{
    segment->header = type | MALLOC_BLOCK_HEAD;
    segment->segmentEnd = end;

    if (type == MALLOC_FREE_BLOCK)
    {
        segment->nextSegment = 0;
        segment->prevSegment = 0;
    }

    struct HeapSegmentFooter* footer = (struct HeapSegmentFooter*)segment->segmentEnd - 1;
    footer->footer = type | MALLOC_BLOCK_FOOT;
    footer->header = segment;
}

void heapInit(void* heapStart, void* heapEnd)
{
    gFirstFreeSegment = (struct HeapSegment*)(((int)heapStart + 7) & ~0x7);
    heapInitBlock(gFirstFreeSegment, heapEnd, MALLOC_FREE_BLOCK);

    gHeapStart = gFirstFreeSegment;
    gHeapEnd = heapEnd;
}

void heapReset() {
    heapInit(gHeapStart, gHeapEnd);
}

void *cacheFreePointer(void* target)
{
    return (void*)(((int)target & 0x0FFFFFFF) | 0xA0000000);
}

void removeHeapSegment(struct HeapSegment* segment)
{
    struct HeapSegment* nextSegment = segment->nextSegment;
    struct HeapSegment* prevSegment = segment->prevSegment;

    if (prevSegment)
    {
        prevSegment->nextSegment = nextSegment;
    }
    else
    {
        gFirstFreeSegment = nextSegment;
    }

    if (nextSegment) {
        nextSegment->prevSegment = prevSegment;
    }
}

void insertHeapSegment(struct HeapSegment* at, struct HeapSegment* segment)
{
    struct HeapSegment* nextSegment;
    struct HeapSegment* prevSegment;

    if (at)
    {
        nextSegment = at->nextSegment;
        prevSegment = at;
    }
    else
    {
        nextSegment = gFirstFreeSegment;
        prevSegment = 0;
    }

    segment->nextSegment = nextSegment;
    segment->prevSegment = prevSegment;

    if (nextSegment)
    {
        nextSegment->prevSegment = segment;
    }

    if (prevSegment)
    {
        prevSegment->nextSegment = segment;
    }
    else
    {
        gFirstFreeSegment = segment;
    }
}

struct HeapSegment* getPrevBlock(struct HeapSegment* at, int type)
{
    struct HeapSegmentFooter* prevFooter = (struct HeapSegmentFooter*)at - 1;

    if ((void*)prevFooter < gHeapStart || (void*)prevFooter >= gHeapEnd)
    {
        return 0;
    }

    if (prevFooter->footer != (MALLOC_BLOCK_FOOT | type))
    {
        return 0;
    }

    return prevFooter->header;
}

struct HeapSegment* getNextBlock(struct HeapSegment* at, int type)
{
    struct HeapUsedSegment* nextHeader = (struct HeapUsedSegment*)at->segmentEnd;

    if ((void*)nextHeader < gHeapStart || (void*)nextHeader >= gHeapEnd)
    {
        return 0;
    }

    if (nextHeader->header != (MALLOC_BLOCK_HEAD | type))
    {
        return 0;
    }

    return (struct HeapSegment*)nextHeader;
}

void *malloc(unsigned int size)
{
    struct HeapSegment* currentSegment;
    int segmentSize;
    // 8 byte align for DMA
    size = (size + 7) & (~0x7);

    size += sizeof(struct HeapUsedSegment);
    size += sizeof(struct HeapSegmentFooter);

    currentSegment = gFirstFreeSegment;

    while (currentSegment)
    {
        segmentSize = (int)currentSegment->segmentEnd - (int)currentSegment;

        if (segmentSize >= size)
        {
            void *newEnd = currentSegment->segmentEnd;
            struct HeapUsedSegment* newSegment;
            if (segmentSize >= size + MIN_HEAP_BLOCK_SIZE)
            {
                newSegment = (struct HeapUsedSegment*)((char*)newEnd - size);

                struct HeapSegment* prevSeg = currentSegment->prevSegment;

                removeHeapSegment(currentSegment);
                heapInitBlock(currentSegment, newSegment, MALLOC_FREE_BLOCK);
                insertHeapSegment(prevSeg, currentSegment);
            }
            else
            {
                removeHeapSegment(currentSegment);

                newSegment = (struct HeapUsedSegment*)currentSegment;
            }

            heapInitBlock((struct HeapSegment*)newSegment, newEnd, MALLOC_USED_BLOCK);

            return newSegment + 1;
        }

        currentSegment = currentSegment->nextSegment;
    }
    
    return 0;
}

void *realloc(void* target, unsigned int size)
{
    if (!target)
    {
        return malloc(size);
    }

    // struct HeapUsedSegment* segment = (struct HeapUsedSegment*)target - 1;

    // void *newBlockEnd = (char*)target + ALIGN_8(size) + sizeof(struct HeapSegmentFooter);
    // u32 actualSize = (u32)segment->segmentEnd - (u32)target;

    // if (newBlockEnd < segment->segmentEnd)
    // {
    //     struct HeapSegment* nextSegment = (struct HeapSegment*)newBlockEnd;

    //     u32 unusedMemory = (char*)segment->segmentEnd - (char*)newBlockEnd;

    //     if (unusedMemory < MIN_HEAP_BLOCK_SIZE)
    //     {
    //         return target;
    //     }

    //     heapInitBlock(nextSegment, segment->segmentEnd, MALLOC_USED_BLOCK);
    //     heapInitBlock((struct HeapSegment*)segment, newBlockEnd, MALLOC_USED_BLOCK);

    //     free(nextSegment);
    // }
    // else if (newBlockEnd == segment->segmentEnd)
    // {
    //     return target;
    // }
    // else
    // {
    //     struct HeapSegment* nextSegment = getNextBlock((struct HeapSegment*)segment, MALLOC_FREE_BLOCK);

    //     if (nextSegment)
    //     {
    //         if (newBlockEnd < nextSegment->segmentEnd)
    //         {
    //             struct HeapSegment* prevFreeSegment = nextSegment->prevSegment;
    //             removeHeapSegment(nextSegment);

    //             if (((char*)nextSegment->segmentEnd - (char*)newBlockEnd) >= MIN_HEAP_BLOCK_SIZE) {
    //                 struct HeapSegment* newSegment = (struct HeapSegment*)newBlockEnd;
    //                 heapInitBlock(newSegment, nextSegment->segmentEnd, MALLOC_FREE_BLOCK);
    //                 heapInitBlock((struct HeapSegment*)segment, newBlockEnd, MALLOC_USED_BLOCK);

    //                 insertHeapSegment(prevFreeSegment, newSegment);
    //             } else {
    //                 heapInitBlock((struct HeapSegment*)segment, nextSegment->segmentEnd, MALLOC_USED_BLOCK);
    //             }

    //             return target;
    //         }
    //     }

    // }

    // as a last resort, find a new chunk of memory for the block
    void *result = malloc(size);
    memCopy(result, target, size);
    free(target);
    return result;
}

void free(void* target)
{
    if ((void*)target < gHeapStart || (void*)target >= gHeapEnd)
    {
        return;
    }

    struct HeapUsedSegment* segment = (struct HeapUsedSegment*)target - 1;  

    struct HeapSegment* prev = getPrevBlock((struct HeapSegment*)segment, MALLOC_FREE_BLOCK);
    struct HeapSegment* next = getNextBlock((struct HeapSegment*)segment, MALLOC_FREE_BLOCK);

    void* segmentEnd = segment->segmentEnd;

    if (prev)
    {
        segment = (struct HeapUsedSegment*)prev;
        removeHeapSegment(prev);
    }

    if (next)
    {
        segmentEnd = next->segmentEnd;
        removeHeapSegment(next);
    }

    heapInitBlock((struct HeapSegment*)segment, segmentEnd, MALLOC_FREE_BLOCK);
    insertHeapSegment(0, (struct HeapSegment*)segment);
}

int calculateHeapSize() {
    return (char*)gHeapEnd - (char*)gHeapStart;
}

int calculateBytesFree()
{
    int result;
    struct HeapSegment* currentSegment; 

    currentSegment = gFirstFreeSegment;
    result = 0;

    while (currentSegment != 0) {
        result += (int)currentSegment->segmentEnd - (int)currentSegment;
        currentSegment = currentSegment->nextSegment;
    }

    return result;
}

int calculateLargestFreeChunk()
{
    int result;
    int current;
    struct HeapSegment* currentSegment; 

    currentSegment = gFirstFreeSegment;
    result = 0;

    while (currentSegment != 0) {
        current = (int)currentSegment->segmentEnd - (int)currentSegment;

        if (current > result)
        {
            result = current;
        }

        currentSegment = currentSegment->nextSegment;
    }

    return result;
}

void zeroMemory(void* memory, int size)
{
    unsigned char* asChar = (unsigned char*)memory;
    
    while (size > 0) {
        *asChar = 0;
        ++asChar;
        --size;
    }
}

void memCopy(void* target, const void* src, int size)
{
    unsigned char* targetAsChar;
    unsigned char* srcAsChar;

    targetAsChar = (unsigned char*)target;
    srcAsChar = (unsigned char*)src;
    
    while (size > 0) {
        *targetAsChar = *srcAsChar;
        ++targetAsChar;
        ++srcAsChar;
        --size;
    }
}
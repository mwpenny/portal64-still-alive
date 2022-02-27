
#include <ultra64.h>
#include "rom.h"

#define DMA_MESSAGE_SIZE    20

OSMesgQueue dmaMessageQ;
OSMesg dmaMessages[DMA_MESSAGE_SIZE];
extern OSPiHandle* gPiHandle;

void romInit() {
    osCreateMesgQueue(&dmaMessageQ, dmaMessages, DMA_MESSAGE_SIZE);
}

void romCopy(const char *src, const char *dest, const int len)
{
    OSIoMesg dmaIoMesgBuf;
    OSMesg dummyMesg;
    
    osInvalDCache((void *)dest, (s32) len);

    dmaIoMesgBuf.hdr.pri      = OS_MESG_PRI_NORMAL;
    dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
    dmaIoMesgBuf.dramAddr     = (void*)dest;
    dmaIoMesgBuf.devAddr      = (u32)src;
    dmaIoMesgBuf.size         = (u32)len;

    osEPiStartDma(gPiHandle, &dmaIoMesgBuf, OS_READ);
    (void) osRecvMesg(&dmaMessageQ, &dummyMesg, OS_MESG_BLOCK);
}
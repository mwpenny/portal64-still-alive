#include "system/cartridge.h"
#include "system/time.h"
#include "util/memory.h"

#include <ultra64.h>

#define PI_QUEUE_SIZE        200
#define DMA_QUEUE_SIZE       20
#define DMA_ASYNC_QUEUE_SIZE 20

#define SRAM_LATENCY         0x5
#define SRAM_PAGE_SIZE       0xD
#define SRAM_REL_DURATION    0x2
#define SRAM_PULSE           0xC
#define SRAM_DOMAIN          PI_DOMAIN2
#define SRAM_BASE_ADDR       0x08000000
#define SRAM_DELAY_USECS     (10 * 1000)

static OSPiHandle*           sCartHandle;
static OSPiHandle            sSramHandle;

static OSMesgQueue           piMessageQ;
static OSMesg                piMessages[PI_QUEUE_SIZE];

static OSMesgQueue           dmaMessageQ;
static OSMesg                dmaMessages[DMA_QUEUE_SIZE];

static OSMesgQueue           asyncDmaMessageQ;
static OSMesg                asyncDmaMessages[DMA_ASYNC_QUEUE_SIZE];
static OSIoMesg              asyncDmaMessageReqs[DMA_ASYNC_QUEUE_SIZE];
static int                   sNextAsyncDmaSlot;
static int                   sPendingAsyncDmaCount;

static void sramHandleInit() {
    sSramHandle.type = DEVICE_TYPE_SRAM;
    sSramHandle.latency = SRAM_LATENCY;
    sSramHandle.pageSize = SRAM_PAGE_SIZE;
    sSramHandle.relDuration = SRAM_REL_DURATION;
    sSramHandle.pulse = SRAM_PULSE;
    sSramHandle.domain = SRAM_DOMAIN;
    sSramHandle.baseAddress = PHYS_TO_K1(SRAM_BASE_ADDR);

    // Unused (speed has no purpose, transferInfo is for 64DD)
    sSramHandle.speed = 0;
    zeroMemory(&sSramHandle.transferInfo, sizeof(sSramHandle.transferInfo));

    osEPiLinkHandle(&sSramHandle);
}

void cartridgeInit() {
    osCreatePiManager(
        (OSPri)OS_PRIORITY_PIMGR,
        &piMessageQ,
        piMessages,
        DMA_QUEUE_SIZE
    );

    sCartHandle = osCartRomInit();
    sramHandleInit();

    osCreateMesgQueue(&dmaMessageQ, dmaMessages, DMA_QUEUE_SIZE);
    osCreateMesgQueue(&asyncDmaMessageQ, asyncDmaMessages, DMA_ASYNC_QUEUE_SIZE);
    sNextAsyncDmaSlot = 0;
    sPendingAsyncDmaCount = 0;
}

void romCopy(const void* romAddr, void* ramAddr, const int size) {
    OSIoMesg msgReq = {
        .hdr.pri      = OS_MESG_PRI_NORMAL,
        .hdr.retQueue = &dmaMessageQ,
        .dramAddr     = ramAddr,
        .devAddr      = (u32)romAddr,
        .size         = size
    };

    osInvalDCache(ramAddr, size);
    osEPiStartDma(sCartHandle, &msgReq, OS_READ);
    osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
}

void romCopyAsync(const void* romAddr, void* ramAddr, const int size) {
    if (sPendingAsyncDmaCount == DMA_ASYNC_QUEUE_SIZE) {
        // Free up a slot
        osRecvMesg(&asyncDmaMessageQ, NULL, OS_MESG_BLOCK);
        --sPendingAsyncDmaCount;
    }

    OSIoMesg* msgReq = &asyncDmaMessageReqs[sNextAsyncDmaSlot];
    sNextAsyncDmaSlot = (sNextAsyncDmaSlot + 1) % DMA_ASYNC_QUEUE_SIZE;

    msgReq->hdr.pri = OS_MESG_PRI_NORMAL;
    msgReq->hdr.retQueue = &asyncDmaMessageQ;
    msgReq->dramAddr = ramAddr;
    msgReq->devAddr = (u32)romAddr;
    msgReq->size = size;

    osInvalDCache(ramAddr, size);
    osEPiStartDma(sCartHandle, msgReq, OS_READ);
    ++sPendingAsyncDmaCount;
}

void romCopyAsyncDrain() {
    while (sPendingAsyncDmaCount) {
        osRecvMesg(&asyncDmaMessageQ, NULL, OS_MESG_BLOCK);
        --sPendingAsyncDmaCount;
    }
}

void sramWrite(void* sramAddr, const void* ramAddr, const int size) {
    OSIoMesg msgReq = {
        .hdr.pri      = OS_MESG_PRI_HIGH,
        .hdr.retQueue = &dmaMessageQ,
        .dramAddr     = (void*)ramAddr,
        .devAddr      = (u32)sramAddr,
        .size         = size
    };

    osWritebackDCache((void*)ramAddr, size);
    if (osEPiStartDma(&sSramHandle, &msgReq, OS_WRITE) == -1)
    {
        // Queue is full. Failing to write save file is non-fatal.
        return;
    }

    osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
    timeUSleep(SRAM_DELAY_USECS);
}

int sramRead(const void* sramAddr, void* ramAddr, const int size) {
    OSIoMesg msgReq = {
        .hdr.pri      = OS_MESG_PRI_HIGH,
        .hdr.retQueue = &dmaMessageQ,
        .dramAddr     = ramAddr,
        .devAddr      = (u32)sramAddr,
        .size         = size
    };

    osInvalDCache(ramAddr, size);
    if (osEPiStartDma(&sSramHandle, &msgReq, OS_READ) == -1)
    {
        // Queue is full. Failing to read save file is non-fatal.
        return 0;
    }

    osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
    timeUSleep(SRAM_DELAY_USECS);
    return 1;
}

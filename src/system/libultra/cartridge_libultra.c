#include "system/cartridge.h"
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

static OSMesgQueue           sPiMessageQ;
static OSMesg                sPiMessages[PI_QUEUE_SIZE];

static OSMesgQueue           sDmaMessageQ;
static OSMesg                sDmaMessages[DMA_QUEUE_SIZE];

static OSMesgQueue           sAsyncDmaMessageQ;
static OSMesg                sAsyncDmaMessages[DMA_ASYNC_QUEUE_SIZE];
static OSIoMesg              sAsyncDmaMessageReqs[DMA_ASYNC_QUEUE_SIZE];
static int                   sNextAsyncDmaSlot;
static int                   sPendingAsyncDmaCount;

static OSMesgQueue           sSleepTimerQ;
static OSMesg                sSleepTimerMsg;

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

static void usleep(u64 usecs) {
    OSTimer timer;
    OSTime countdown = OS_USEC_TO_CYCLES(usecs);

    osSetTimer(&timer, countdown, 0, &sSleepTimerQ, 0);
    osRecvMesg(&sSleepTimerQ, NULL, OS_MESG_BLOCK);
}

void cartridgeInit() {
    osCreatePiManager(
        (OSPri)OS_PRIORITY_PIMGR,
        &sPiMessageQ,
        sPiMessages,
        DMA_QUEUE_SIZE
    );

    sCartHandle = osCartRomInit();
    sramHandleInit();

    osCreateMesgQueue(&sDmaMessageQ, sDmaMessages, DMA_QUEUE_SIZE);
    osCreateMesgQueue(&sAsyncDmaMessageQ, sAsyncDmaMessages, DMA_ASYNC_QUEUE_SIZE);
    osCreateMesgQueue(&sSleepTimerQ, &sSleepTimerMsg, 1);
    sNextAsyncDmaSlot = 0;
    sPendingAsyncDmaCount = 0;
}

void romCopy(const void* romAddr, void* ramAddr, const int size) {
    OSIoMesg msgReq = {
        .hdr.pri      = OS_MESG_PRI_NORMAL,
        .hdr.retQueue = &sDmaMessageQ,
        .dramAddr     = ramAddr,
        .devAddr      = (u32)romAddr,
        .size         = size
    };

    osInvalDCache(ramAddr, size);
    osEPiStartDma(sCartHandle, &msgReq, OS_READ);
    osRecvMesg(&sDmaMessageQ, NULL, OS_MESG_BLOCK);
}

void romCopyAsync(const void* romAddr, void* ramAddr, const int size) {
    if (sPendingAsyncDmaCount == DMA_ASYNC_QUEUE_SIZE) {
        // Free up a slot
        osRecvMesg(&sAsyncDmaMessageQ, NULL, OS_MESG_BLOCK);
        --sPendingAsyncDmaCount;
    }

    OSIoMesg* msgReq = &sAsyncDmaMessageReqs[sNextAsyncDmaSlot];
    sNextAsyncDmaSlot = (sNextAsyncDmaSlot + 1) % DMA_ASYNC_QUEUE_SIZE;

    msgReq->hdr.pri = OS_MESG_PRI_NORMAL;
    msgReq->hdr.retQueue = &sAsyncDmaMessageQ;
    msgReq->dramAddr = ramAddr;
    msgReq->devAddr = (u32)romAddr;
    msgReq->size = size;

    osInvalDCache(ramAddr, size);
    osEPiStartDma(sCartHandle, msgReq, OS_READ);
    ++sPendingAsyncDmaCount;
}

void romCopyAsyncDrain() {
    while (sPendingAsyncDmaCount) {
        osRecvMesg(&sAsyncDmaMessageQ, NULL, OS_MESG_BLOCK);
        --sPendingAsyncDmaCount;
    }
}

void sramWrite(void* sramAddr, const void* ramAddr, const int size) {
    OSIoMesg msgReq = {
        .hdr.pri      = OS_MESG_PRI_HIGH,
        .hdr.retQueue = &sDmaMessageQ,
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

    osRecvMesg(&sDmaMessageQ, NULL, OS_MESG_BLOCK);
    usleep(SRAM_DELAY_USECS);
}

int sramRead(const void* sramAddr, void* ramAddr, const int size) {
    OSIoMesg msgReq = {
        .hdr.pri      = OS_MESG_PRI_HIGH,
        .hdr.retQueue = &sDmaMessageQ,
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

    osRecvMesg(&sDmaMessageQ, NULL, OS_MESG_BLOCK);
    usleep(SRAM_DELAY_USECS);
    return 1;
}

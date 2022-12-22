
#include <assert.h>
#include "audio.h"
#include "defs.h"

extern OSPiHandle	*gPiHandle;

/****  type define's for structures unique to audiomgr ****/
typedef union {    

    struct {
        short     type;
    } gen;

    struct {
        short     type;
        struct    AudioInfo_s *info;
    } done;
    
    OSScMsg       app;
    
} AudioMsg;

typedef struct AudioInfo_s {
    short         *data;          /* Output data pointer */
    short         frameSamples;   /* # of samples synthesized in this frame */
    OSScTask      task;           /* scheduler structure */
    AudioMsg      msg;            /* completion message */
} AudioInfo;

typedef struct {
    Acmd          *ACMDList[NUM_ACMD_LISTS];
    AudioInfo     *audioInfo[NUM_OUTPUT_BUFFERS];
    OSThread      thread;
    OSMesgQueue   audioFrameMsgQ;
    OSMesg        audioFrameMsgBuf[MAX_AUDIO_MESGS];
    OSMesgQueue   audioReplyMsgQ;
    OSMesg        audioReplyMsgBuf[MAX_AUDIO_MESGS];
    ALGlobals     g;
} AMAudioMgr;

typedef struct 
{
    ALLink        node;
    u32           startAddr;
    u32           lastFrame;
    char          *ptr;
} AMDMABuffer;

typedef struct 
{
    u8            initialized;
    AMDMABuffer   *firstUsed;
    AMDMABuffer   *firstFree;
} AMDMAState;


/**** audio manager globals ****/
extern OSSched         scheduler;
extern OSMesgQueue     *schedulerCommandQueue;

AMAudioMgr      __am;
static u64      audioStack[AUDIO_STACKSIZE/sizeof(u64)];

AMDMAState      dmaState;
AMDMABuffer     dmaBuffs[NUM_DMA_BUFFERS];
u32             audFrameCt = 0;
u32             nextDMA = 0;
u32             curAcmdList = 0;
u32             minFrameSize;
u32             frameSize;
u32             maxFrameSize;
u32             maxRSPCmds;

/** Queues and storage for use with audio DMA's ****/
OSIoMesg        audDMAIOMesgBuf[NUM_DMA_MESSAGES];
OSMesgQueue     audDMAMessageQ;
OSMesg          audDMAMessageBuf[NUM_DMA_MESSAGES];

/**** private routines ****/
static void __amMain(void *arg);
static s32  __amDMA(s32 addr, s32 len, void *state);
static ALDMAproc __amDmaNew(AMDMAState **state);
static u32  __amHandleFrameMsg(AudioInfo *, AudioInfo *);
static void __amHandleDoneMsg(AudioInfo *);
static void __clearAudioDMA(void);


/******************************************************************************
 * Audio Manager API
 *****************************************************************************/
void amCreateAudioMgr(ALSynConfig *c, OSPri pri, amConfig *amc, int fps)
{
    u32     i;
    f32     fsize;

    dmaState.initialized = FALSE;	/* Reset this before the first call to __amDmaNew */

    c->dmaproc    = __amDmaNew;    
    c->outputRate = osAiSetFrequency(amc->outputRate);

    /*
     * Calculate the frame sample parameters from the
     * video field rate and the output rate
     */
    fsize = (f32) amc->framesPerField * c->outputRate / (f32) fps;
    frameSize = (s32) fsize;
    if (frameSize < fsize)
        frameSize++;
    if (frameSize & 0xf)
        frameSize = (frameSize & ~0xf) + 0x10;
    minFrameSize = frameSize - 16;
    maxFrameSize = frameSize + EXTRA_SAMPLES + 16;

    alInit(&__am.g, c);

    dmaBuffs[0].node.prev = 0; 
    dmaBuffs[0].node.next = 0;
    for (i=0; i<NUM_DMA_BUFFERS-1; i++)
    {
        alLink((ALLink*)&dmaBuffs[i+1],(ALLink*)&dmaBuffs[i]);
        dmaBuffs[i].ptr = alHeapAlloc(c->heap, 1, DMA_BUFFER_LENGTH);
    }
    /* last buffer already linked, but still needs buffer */
    dmaBuffs[i].ptr = alHeapAlloc(c->heap, 1, DMA_BUFFER_LENGTH);
    
    for(i=0;i<NUM_ACMD_LISTS;i++)
        __am.ACMDList[i] = (Acmd*)alHeapAlloc(c->heap, 1, 
                            amc->maxACMDSize * sizeof(Acmd));

    maxRSPCmds = amc->maxACMDSize;

    /**** initialize the done messages ****/
    for (i = 0; i < NUM_OUTPUT_BUFFERS; i++) 
    {
        __am.audioInfo[i] = (AudioInfo *)alHeapAlloc(c->heap, 1,
                                                     sizeof(AudioInfo));
        __am.audioInfo[i]->msg.done.type = OS_SC_DONE_MSG;
        __am.audioInfo[i]->msg.done.info = __am.audioInfo[i];
        __am.audioInfo[i]->data = alHeapAlloc(c->heap, 1, 4*maxFrameSize);
    }    
    
    osCreateMesgQueue(&__am.audioReplyMsgQ, __am.audioReplyMsgBuf, MAX_AUDIO_MESGS);
    osCreateMesgQueue(&__am.audioFrameMsgQ, __am.audioFrameMsgBuf, MAX_AUDIO_MESGS);
    osCreateMesgQueue(&audDMAMessageQ, audDMAMessageBuf, NUM_DMA_MESSAGES);

    osCreateThread(&__am.thread, 3, __amMain, 0,
                   (void *)(audioStack+AUDIO_STACKSIZE/sizeof(u64)), pri);
    osStartThread(&__am.thread);
}

static void __amMain(void *arg) 
{
    u32         validTask;
    u32         done = 0;
    AudioMsg    *msg;
    AudioInfo   *lastInfo = 0;
    OSScClient  client;


    osScAddClient(&scheduler, &client, &__am.audioFrameMsgQ);
    
    while (!done) 
    {
        (void) osRecvMesg(&__am.audioFrameMsgQ, (OSMesg *)&msg, OS_MESG_BLOCK);

        switch (msg->gen.type) 
        {
            case (OS_SC_RETRACE_MSG):
                validTask = __amHandleFrameMsg(__am.audioInfo[audFrameCt % 3],
                                               lastInfo);
                if(validTask)
                {
                    /* wait for done message */
                    osRecvMesg(&__am.audioReplyMsgQ, (OSMesg *)&msg, 
                               OS_MESG_BLOCK);
                    __amHandleDoneMsg(msg->done.info);
                    lastInfo = msg->done.info;
                }
                break;

            case (OS_SC_PRE_NMI_MSG):
                /* what should we really do here? quit? ramp down volume? */
                break;

            case (QUIT_MSG):
                done = 1;
                break;

            default:
                break;
        }        
    }
    
    alClose(&__am.g);
}

static u32 __amHandleFrameMsg(AudioInfo *info, AudioInfo *lastInfo)
{
    s16 *audioPtr;
    Acmd *cmdp;
    s32 cmdLen;
    int samplesLeft = 0;
    OSScTask *t;

    
    __clearAudioDMA(); /* call once a frame, before doing alAudioFrame */

    audioPtr = (s16 *) osVirtualToPhysical(info->data);
    
    if (lastInfo)
        osAiSetNextBuffer(lastInfo->data, lastInfo->frameSamples<<2);

    
    /* calculate how many samples needed for this frame to keep the DAC full */
    /* this will vary slightly frame to frame, must recalculate every frame */
    samplesLeft = osAiGetLength() >> 2; /* divide by four, to convert bytes */
                                        /* to stereo 16 bit samples */
    info->frameSamples = (16 + (frameSize - samplesLeft + EXTRA_SAMPLES)) & ~0xf;
    if(info->frameSamples < minFrameSize)
        info->frameSamples = minFrameSize;

    cmdp = alAudioFrame(__am.ACMDList[curAcmdList], &cmdLen, audioPtr,
                        info->frameSamples);

    assert(cmdLen <= maxRSPCmds);
    
    if(cmdLen == 0)  /* no task produced, return zero to show no valid task */
        return 0;

    t = &info->task;
    
    t->next      = 0;                    /* paranoia */
    t->msgQ      = &__am.audioReplyMsgQ; /* reply to when finished */
    t->msg       = (OSMesg)&info->msg;   /* reply with this message */
    t->flags     = OS_SC_NEEDS_RSP;
    
    t->list.t.data_ptr    = (u64 *) __am.ACMDList[curAcmdList];
    t->list.t.data_size   = (cmdp - __am.ACMDList[curAcmdList]) * sizeof(Acmd);
    t->list.t.type  = M_AUDTASK;
    t->list.t.ucode_boot = (u64 *)rspbootTextStart;
    t->list.t.ucode_boot_size =
        ((int) rspbootTextEnd - (int) rspbootTextStart);
    t->list.t.flags = 0;
    t->list.t.ucode = (u64 *) aspMainTextStart;
    t->list.t.ucode_data = (u64 *) aspMainDataStart;
    t->list.t.ucode_data_size = SP_UCODE_DATA_SIZE;
    t->list.t.dram_stack = (u64 *) NULL;
    t->list.t.dram_stack_size = 0;
    t->list.t.output_buff = (u64 *) NULL;
    t->list.t.output_buff_size = 0;
    t->list.t.yield_data_ptr = NULL;
    t->list.t.yield_data_size = 0;
#ifndef	STOP_AUDIO
    osSendMesg(schedulerCommandQueue, (OSMesg) t, OS_MESG_BLOCK);
#endif
    curAcmdList ^= 1; /* swap which acmd list you use each frame */    
    
    return 1;
}

/******************************************************************************
 *
 * __amHandleDoneMsg. Really just debugging info in this frame. Checks
 * to make sure we completed before we were out of samples.
 *
 *****************************************************************************/
static void __amHandleDoneMsg(AudioInfo *info) 
{
    s32    samplesLeft;
    static int firstTime = 1;

    samplesLeft = osAiGetLength()>>2;
    if (samplesLeft == 0 && !firstTime) 
    {
        PRINTF("audio: ai out of samples\n");    
        firstTime = 0;
    }
}

s32 __amDMA(s32 addr, s32 len, void *state)
{
    void            *foundBuffer;
    s32             delta, addrEnd, buffEnd;
    AMDMABuffer     *dmaPtr, *lastDmaPtr;
    
    lastDmaPtr = 0;
    dmaPtr = dmaState.firstUsed;
    addrEnd = addr+len;

    /* first check to see if a currently existing buffer contains the
       sample that you need.  */
    
    while(dmaPtr)
    {
        buffEnd = dmaPtr->startAddr + DMA_BUFFER_LENGTH;
        if(dmaPtr->startAddr > addr) /* since buffers are ordered */
            break;                   /* abort if past possible */

        else if(addrEnd <= buffEnd) /* yes, found a buffer with samples */
        {
            dmaPtr->lastFrame = audFrameCt; /* mark it used */
            foundBuffer = dmaPtr->ptr + addr - dmaPtr->startAddr;
            return (int) osVirtualToPhysical(foundBuffer);
        }
        lastDmaPtr = dmaPtr;
        dmaPtr = (AMDMABuffer*)dmaPtr->node.next;
    }

    /* get here, and you didn't find a buffer, so dma a new one */

    /* get a buffer from the free list */
    dmaPtr = dmaState.firstFree;

    /* 
     * if you get here and dmaPtr is null, send back the a bogus
     * pointer, it's better than nothing
     */
    if(!dmaPtr)
	return osVirtualToPhysical(dmaState.firstUsed);

    dmaState.firstFree = (AMDMABuffer*)dmaPtr->node.next;
    alUnlink((ALLink*)dmaPtr);

    /* add it to the used list */
    if(lastDmaPtr) /* if you have other dmabuffers used, add this one */
    {              /* to the list, after the last one checked above */
        alLink((ALLink*)dmaPtr,(ALLink*)lastDmaPtr);
    }
    else if(dmaState.firstUsed) /* if this buffer is before any others */
    {                           /* jam at begining of list */ 
        lastDmaPtr = dmaState.firstUsed;
        dmaState.firstUsed = dmaPtr;
        dmaPtr->node.next = (ALLink*)lastDmaPtr;
        dmaPtr->node.prev = 0;
        lastDmaPtr->node.prev = (ALLink*)dmaPtr;
    }
    else /* no buffers in list, this is the first one */
    {
        dmaState.firstUsed = dmaPtr;
        dmaPtr->node.next = 0;
        dmaPtr->node.prev = 0;
    }
    
    foundBuffer = dmaPtr->ptr;
    delta = addr & 0x1;
    addr -= delta;
    dmaPtr->startAddr = addr;
    dmaPtr->lastFrame = audFrameCt;  /* mark it */

    audDMAIOMesgBuf[nextDMA].hdr.pri      = OS_MESG_PRI_NORMAL;
    audDMAIOMesgBuf[nextDMA].hdr.retQueue = &audDMAMessageQ;
    audDMAIOMesgBuf[nextDMA].dramAddr     = foundBuffer;
    audDMAIOMesgBuf[nextDMA].devAddr      = (u32)addr;
    audDMAIOMesgBuf[nextDMA].size         = DMA_BUFFER_LENGTH;

    osEPiStartDma(gPiHandle, &audDMAIOMesgBuf[nextDMA++], OS_READ);

    return (int) osVirtualToPhysical(foundBuffer) + delta;
}

ALDMAproc __amDmaNew(AMDMAState **state)
{    
    if(!dmaState.initialized)  /* only do this once */
    {
        dmaState.firstUsed = 0;
        dmaState.firstFree = &dmaBuffs[0];
        dmaState.initialized = 1;
    }

    *state = &dmaState;  /* state is never used in this case */

    return __amDMA;
}

/******************************************************************************
 *
 * __clearAudioDMA.  Routine to move dma buffers back to the unused list.
 * First clear out your dma messageQ. Then check each buffer to see when
 * it was last used. If that was more than FRAME_LAG frames ago, move it
 * back to the unused list. 
 *
 *****************************************************************************/
static void __clearAudioDMA(void)
{
    u32          i;
    OSIoMesg     *iomsg;
    AMDMABuffer  *dmaPtr,*nextPtr;
    
    /*
     * Don't block here. If dma's aren't complete, you've had an audio
     * overrun. (Bad news, but go for it anyway, and try and recover.
     */
    for (i=0; i<nextDMA; i++)
    {
        if (osRecvMesg(&audDMAMessageQ,(OSMesg *)&iomsg,OS_MESG_NOBLOCK) == -1)
            PRINTF("Dma not done\n");
    }

    
    dmaPtr = dmaState.firstUsed;
    while(dmaPtr)
    {
        nextPtr = (AMDMABuffer*)dmaPtr->node.next;

        /* remove old dma's from list */
        /* Can change FRAME_LAG value.  Should be at least one.  */
        /* Larger values mean more buffers needed, but fewer DMA's */
        if(dmaPtr->lastFrame + FRAME_LAG  < audFrameCt) 
        {
            if(dmaState.firstUsed == dmaPtr)
                dmaState.firstUsed = (AMDMABuffer*)dmaPtr->node.next;
            alUnlink((ALLink*)dmaPtr);
            if(dmaState.firstFree)
                alLink((ALLink*)dmaPtr,(ALLink*)dmaState.firstFree);
            else
            {
                dmaState.firstFree = dmaPtr;
                dmaPtr->node.next = 0;
                dmaPtr->node.prev = 0;
            }
        }
        dmaPtr = nextPtr;
    }
    
    nextDMA = 0;  /* reset */
    audFrameCt++;
}


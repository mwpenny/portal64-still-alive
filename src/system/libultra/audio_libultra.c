#include "system/audio.h"

#include "math/mathf.h"
#include "system/cartridge.h"
#include "threads_libultra.h"
#include "util/frame_time.h"
#include "util/memory.h"

#include <assert.h>
#include <sched.h>
#include <ultra64.h>

#define AUDIO_HEAP_SIZE_BYTES       300000

// Flags used to manage the state of each available voice
#define AUDIO_VOICE_FLAG_PLAYING    (1 << 0)  // Playback is in progress
#define AUDIO_VOICE_FLAG_DID_OUTPUT (1 << 1)  // Samples have been output
#define AUDIO_VOICE_FLAG_PAUSED     (1 << 2)  // Playback is paused

// The number of frames of lag that can be detected.
//
// Allows immediately generating more samples instead of waiting for next frame.
#define AUDIO_FRAME_QUEUE_SIZE      32

// Events represent queued voice changes (start/stop, paramater change, etc.).
//
// They can accumulate over multiple frames, such as with sound stop events.
// Each event uses 28 bytes of the audio heap.
#define AUDIO_MAX_EVENTS            256

// Sound updates are created in response to events processed during generation
// of a frame's audio command list.
//
// Each update uses 28 bytes of the audio heap.
#define AUDIO_MAX_UPDATES           64

// Audio command lists are generated from audio updates, then sent to the RSP to
// be converted into audio samples.
//
// A single update can write multiple command list entries.
// Each command list entry uses 8 bytes of the audio heap.
#define AUDIO_MAX_COMMAND_LIST_SIZE 2048

// Sound samples are requested during audio command list generation.
//
// Those not in a buffer are DMAed in. Otherwise a previous buffer is re-used.
// A higher DMA size increases the likelihood of re-use, which reduces the
// number of DMAs.
#define AUDIO_DMA_BUFFER_COUNT      30
#define AUDIO_MAX_DMA_TRANSFERS     30
#define AUDIO_DMA_SIZE              2048

// The N64 DAC can queue up to 2 sample buffers (16-bit stereo).
// Use 3 so another can be written while 2 are queued.
#define AUDIO_SAMPLE_BUFFER_COUNT   3
#define AUDIO_SAMPLE_SIZE           (sizeof(s16) * 2)

// Target sample count padding is added to cover audio code execution time
// Minimum sample count decrease is subtracted to avoid unbounded sample growth
#define AUDIO_TARGET_SAMPLE_PADDING (6 * 16)
#define AUDIO_MIN_SAMPLE_DECREASE   (1 * 16)

// Stack size for the audio thread
#define AUDIO_STACK_SIZE_BYTES      2048

// Format of sfz2n64 output
struct SoundArray {
    u32 soundCount;
    ALSound* sounds[];
};

struct VoiceState {
    float timeRemaining;
    float pitch;
    s16 volume;
    u8 flags;
};

struct DmaBufferMetadata {
    u32 address;
    u32 framesOld;
};

static ALHeap                   sAudioHeap;

extern char                     _soundsSegmentRomStart[];
extern char                     _soundsSegmentRomEnd[];
extern char                     _soundsTblSegmentRomStart[];
static struct SoundArray*       sSoundClipArray;
static struct VoiceState*       sVoiceStates;
static int                      sMaxVoices;

static ALGlobals                sAudioGlobals;
static ALSndPlayer              sSoundPlayer;

static u8*                      sDmaBuffers[AUDIO_DMA_BUFFER_COUNT];
static struct DmaBufferMetadata sDmaBufferMetadata[AUDIO_DMA_BUFFER_COUNT];

static OSPiHandle*              sPiHandle;
static OSMesgQueue              sAudioDmaMessageQueue;
static OSMesg                   sAudioDmaMessages[AUDIO_MAX_DMA_TRANSFERS];
static OSIoMesg                 sAudioDmaMessageReqs[AUDIO_MAX_DMA_TRANSFERS];
static int                      sNextDmaSlot;
static int                      sActiveDmaCount;

extern OSSched                  scheduler;
static OSMesgQueue*             sSchedulerTaskQueue;
static OSMesgQueue              sSchedulerTaskDoneQueue;
static OSScMsg                  sSchedulerTaskDoneMsg;

static OSThread                 sAudioThread;
static u64                      sAudioThreadStack[AUDIO_STACK_SIZE_BYTES / sizeof(u64)];

#define OFFSET_POINTER(base, offset) (void*)((unsigned)(base) + (unsigned)(offset))
#define OFFSET_POINTER_NULLABLE(base, offset) ((offset) ? OFFSET_POINTER(base, offset) : NULL)

struct SoundArray* audioLoadSounds() {
    int soundDataSize = _soundsSegmentRomEnd - _soundsSegmentRomStart;
    struct SoundArray* soundArray = alHeapAlloc(&sAudioHeap, 1, soundDataSize);
    romCopy(_soundsSegmentRomStart, soundArray, soundDataSize);

    for (int i = 0; i < soundArray->soundCount; ++i) {
        // Serialized format stores offsets. Convert to pointers.
        soundArray->sounds[i] = OFFSET_POINTER(soundArray, soundArray->sounds[i]);

        ALSound* sound = soundArray->sounds[i];
        sound->envelope = OFFSET_POINTER(soundArray, sound->envelope);
        sound->keyMap = OFFSET_POINTER(soundArray, sound->keyMap);
        sound->wavetable = OFFSET_POINTER(soundArray, sound->wavetable);

        ALWaveTable* waveTable = sound->wavetable;
        waveTable->base = OFFSET_POINTER(_soundsTblSegmentRomStart, waveTable->base);

        if (waveTable->type == AL_ADPCM_WAVE) {
            waveTable->waveInfo.adpcmWave.book = OFFSET_POINTER(soundArray, waveTable->waveInfo.adpcmWave.book);
            waveTable->waveInfo.adpcmWave.loop = OFFSET_POINTER_NULLABLE(soundArray, waveTable->waveInfo.adpcmWave.loop);
        } else if (waveTable->type == AL_RAW16_WAVE) {
            waveTable->waveInfo.rawWave.loop = OFFSET_POINTER_NULLABLE(soundArray, waveTable->waveInfo.rawWave.loop);
        }

        // Libultra doesn't provide a good way to pause sounds, or to start
        // sounds partway through. So to "pause" sounds we set the pitch to 0.
        // Unfortunately mid-playback pitch changes don't affect the envelope
        // and so sounds will end at the same time regardless of the pause.
        //
        // This is a bit of a hack to get around the problem. All sounds are
        // given infinite duration and we will stop them at the proper time.
        sound->envelope->decayTime = -1;
    }

    return soundArray;
}

static s32 audioRequestDma(s32 addr, s32 len, void*) {
    s32 endAddr = addr + len;
    u32 bufferIndex = 0;

    for (int i = 0; i < AUDIO_DMA_BUFFER_COUNT; ++i) {
        struct DmaBufferMetadata* bufInfo = &sDmaBufferMetadata[i];

        // If the data already exists in a buffer we can avoid a DMA
        if (bufInfo->address <= addr && endAddr <= (bufInfo->address + AUDIO_DMA_SIZE)) {
            s32 offset = addr - bufInfo->address;
            bufInfo->framesOld = 0;
            return K0_TO_PHYS(sDmaBuffers[i] + offset);
        }

        // Evict least recently used buffer
        if (bufInfo->framesOld > sDmaBufferMetadata[bufferIndex].framesOld) {
            bufferIndex = i;
        }
    }

    struct DmaBufferMetadata* bufInfo = &sDmaBufferMetadata[bufferIndex];
    if (bufInfo->framesOld == 0 || sActiveDmaCount >= AUDIO_MAX_DMA_TRANSFERS) {
        // No free buffers! This will cause audio glitching.
        return K0_TO_PHYS(sDmaBuffers[0]);
    }

    // DMA source must be 2-byte aligned
    bufInfo->address = addr & ~1;
    bufInfo->framesOld = 0;

    OSIoMesg* msgReq = &sAudioDmaMessageReqs[sNextDmaSlot];
    msgReq->hdr.pri = OS_MESG_PRI_NORMAL;
    msgReq->hdr.retQueue = &sAudioDmaMessageQueue;
    msgReq->dramAddr = sDmaBuffers[bufferIndex];
    msgReq->devAddr = bufInfo->address;
    msgReq->size = AUDIO_DMA_SIZE;

    // RSP will read memory directly so no need to invalidate data cache
    osEPiStartDma(sPiHandle, msgReq, OS_READ);
    sNextDmaSlot = (sNextDmaSlot + 1) % AUDIO_MAX_DMA_TRANSFERS;
    ++sActiveDmaCount;

    // Add back offset if needed to round down for 2-byte alignment
    return K0_TO_PHYS(msgReq->dramAddr + (addr & 1));
}

static ALDMAproc audioCreateDmaCallback(void*) {
    return audioRequestDma;
}

static void audioExecuteCommandList(Acmd* commandList, u32 commandListSize) {
    assert(commandListSize > 0);

    // Initialize and run RSP task which converts audio command list to samples
    static OSScTask task;
    task.flags = OS_SC_NEEDS_RSP;
    task.msgQ = &sSchedulerTaskDoneQueue;
    task.msg = NULL;

    task.list.t.type = M_AUDTASK;
    task.list.t.flags = 0;
    task.list.t.ucode_boot = (u64*)rspbootTextStart;
    task.list.t.ucode_boot_size = ((u32)rspbootTextEnd - (u32)rspbootTextStart);
    task.list.t.ucode = (u64*)aspMainTextStart;
    task.list.t.ucode_size = SP_UCODE_SIZE;
    task.list.t.ucode_data = (u64*)aspMainDataStart;
    task.list.t.ucode_data_size = SP_UCODE_DATA_SIZE;
    task.list.t.dram_stack = NULL;
    task.list.t.dram_stack_size = 0;
    task.list.t.output_buff = NULL;
    task.list.t.output_buff_size = NULL;
    task.list.t.data_ptr = (u64*)commandList;
    task.list.t.data_size = commandListSize * sizeof(Acmd);
    task.list.t.yield_data_ptr = NULL;
    task.list.t.yield_data_size = 0;

    osSendMesg(sSchedulerTaskQueue, (OSMesg)&task, OS_MESG_BLOCK);
    osRecvMesg(&sSchedulerTaskDoneQueue, NULL, OS_MESG_BLOCK);
}

static void audioUpdateDma() {
    u32 activeDmas = sActiveDmaCount;
    for (u32 i = 0; i < AUDIO_DMA_BUFFER_COUNT; ++i) {
        ++sDmaBufferMetadata[i].framesOld;

        if (i < activeDmas) {
            if (osRecvMesg(&sAudioDmaMessageQueue, NULL, OS_MESG_NOBLOCK) != 0) {
                // DMA not done! This will cause audio glitching.
                continue;
            }

            --sActiveDmaCount;
        }
    }
}

static int audioGenerateSamples(Acmd* commandListBuffer, u8* sampleBuffer, int sampleCount) {
    // RSP will read memory directly so no need to invalidate data cache
    s32 cmdListSize = 0;
    alAudioFrame(
        commandListBuffer,
        &cmdListSize,
        (s16*)K0_TO_PHYS(sampleBuffer),
        sampleCount
    );

    assert(cmdListSize <= AUDIO_MAX_COMMAND_LIST_SIZE);

    if (cmdListSize == 0) {
        // Nothing to write. Must not run the RSP task or the game will hang.
        return 0;
    }

    audioExecuteCommandList(commandListBuffer, cmdListSize);
    audioUpdateDma();
    return 1;
}

static void audioThread(void* arg) {
    OSMesgQueue frameQueue;
    OSMesg frameMsgBuf[AUDIO_FRAME_QUEUE_SIZE];
    OSScClient audioClient;
    osCreateMesgQueue(&frameQueue, frameMsgBuf, AUDIO_FRAME_QUEUE_SIZE);
    osScAddClient(&scheduler, &audioClient, &frameQueue);

    Acmd* commandListBuffer = alHeapAlloc(
        &sAudioHeap,
        1,
        sizeof(Acmd) * AUDIO_MAX_COMMAND_LIST_SIZE
    );

    // It takes time to execute the audio thread and queue sample buffers to be
    // output, so generate some extra samples to cover it.
    //
    // The minimum sample count ensures no gaps between frames. It is less then
    // one frame so accumulated samples can be gradually drained.
    int fps = (osTvType == OS_TV_PAL) ? 50 : 60;
    int frameSampleCount = ALIGN_16((int)ceilf(AUDIO_OUTPUT_HZ / (float)fps));
    int targetSampleCount = frameSampleCount + AUDIO_TARGET_SAMPLE_PADDING;
    int minSampleCount = frameSampleCount - AUDIO_MIN_SAMPLE_DECREASE;

    u8* sampleBuffers[AUDIO_SAMPLE_BUFFER_COUNT];
    for (int i = 0; i < AUDIO_SAMPLE_BUFFER_COUNT; ++i) {
        sampleBuffers[i] = alHeapAlloc(
            &sAudioHeap,
            1,
            targetSampleCount * AUDIO_SAMPLE_SIZE
        );
    }

    int nextSampleBuffer = 0;
    int nextSampleCount = 0;

    while (1) {
        // Wait for end of frame
        OSScMsg* msg;
        osRecvMesg(&frameQueue, (OSMesg*)&msg, OS_MESG_BLOCK);
        if (msg->type != OS_SC_RETRACE_MSG) {
            continue;
        }

        // If the DAC is full, we either generated too many samples or
        // are catching up after being behind. Just skip in both cases.
        //
        // In practice there should always be room due the sample count
        // calculation below and this being a high-priority thread.
        if (nextSampleCount > 0 && !(osAiGetStatus() & AI_STATUS_FIFO_FULL)) {
            osAiSetNextBuffer(sampleBuffers[nextSampleBuffer], nextSampleCount * AUDIO_SAMPLE_SIZE);
            nextSampleBuffer = (nextSampleBuffer + 1) % AUDIO_SAMPLE_BUFFER_COUNT;
        }

        // Keep target number of samples in flight (i.e., modulate amount remaining)
        int samplesRemaining = osAiGetLength() / AUDIO_SAMPLE_SIZE;
        int targetSampleDelta = ALIGN_16(targetSampleCount - samplesRemaining);
        int sampleCount = MAX(targetSampleDelta, minSampleCount);

        if (audioGenerateSamples(commandListBuffer, sampleBuffers[nextSampleBuffer], sampleCount)) {
            nextSampleCount = sampleCount;
        } else {
            nextSampleCount = 0;
        }
    }
}

void* audioInit(void* heapEnd, int maxVoices) {
    void* heapStart = heapEnd - AUDIO_HEAP_SIZE_BYTES;
    alHeapInit(&sAudioHeap, heapStart, AUDIO_HEAP_SIZE_BYTES);

    sSoundClipArray = audioLoadSounds();
    sVoiceStates = alHeapAlloc(&sAudioHeap, 1, maxVoices * sizeof(struct VoiceState));
    sMaxVoices = maxVoices;

    // Library
    ALSynConfig audioConfig = {
        .maxVVoices = maxVoices,
        .maxPVoices = maxVoices,
        .maxUpdates = AUDIO_MAX_UPDATES,
        .dmaproc    = audioCreateDmaCallback,
        .heap       = &sAudioHeap,
        .outputRate = osAiSetFrequency(AUDIO_OUTPUT_HZ),
        .fxType     = AL_FX_BIGROOM
    };
    alInit(&sAudioGlobals, &audioConfig);

    ALSndpConfig soundPlayerConfig = {
        .maxSounds  = maxVoices,
        .maxEvents  = AUDIO_MAX_EVENTS,
        .heap       = &sAudioHeap
    };
    alSndpNew(&sSoundPlayer, &soundPlayerConfig);

    // DMA
    sPiHandle = osCartRomInit();
    osCreateMesgQueue(&sAudioDmaMessageQueue, sAudioDmaMessages, AUDIO_MAX_DMA_TRANSFERS);
    for (int i = 0; i < AUDIO_DMA_BUFFER_COUNT; ++i) {
        sDmaBuffers[i] = alHeapAlloc(&sAudioHeap, 1, AUDIO_DMA_SIZE);
        sDmaBufferMetadata[i].address = 0;
        sDmaBufferMetadata[i].framesOld = 0;
    }

    sNextDmaSlot = 0;
    sActiveDmaCount = 0;

    // Thread
    sSchedulerTaskQueue = osScGetCmdQ(&scheduler);
    osCreateMesgQueue(&sSchedulerTaskDoneQueue, (OSMesg*)&sSchedulerTaskDoneMsg, 1);

    osCreateThread(
        &sAudioThread,
        AUDIO_THREAD_ID,
        audioThread,
        0,
        sAudioThreadStack + (AUDIO_STACK_SIZE_BYTES / sizeof(u64)),
        (OSPri)AUDIO_PRIORITY
    );
    osStartThread(&sAudioThread);

    return heapStart;
}

void audioUpdate() {
    for (int i = 0; i < sMaxVoices; ++i) {
        struct VoiceState* voiceState = &sVoiceStates[i];
        if ((voiceState->flags & (AUDIO_VOICE_FLAG_PLAYING | AUDIO_VOICE_FLAG_PAUSED)) != AUDIO_VOICE_FLAG_PLAYING) {
            continue;
        }

        alSndpSetSound(&sSoundPlayer, i);

        // Check if it is time for the sound to stop. We must manage this
        // ourselves due how sounds are paused. See audioLoadSounds().
        //
        // Do this before the started check below to avoid ending early.
        if ((voiceState->flags & AUDIO_VOICE_FLAG_DID_OUTPUT) && voiceState->timeRemaining > 0.0f) {
            if (voiceState->timeRemaining > FIXED_DELTA_TIME) {
                voiceState->timeRemaining -= FIXED_DELTA_TIME;
            } else {
                voiceState->timeRemaining = 0.0f;
                alSndpStop(&sSoundPlayer);
            }
        }

        int state = alSndpGetState(&sSoundPlayer);
        if (state == AL_STOPPED && (voiceState->flags & AUDIO_VOICE_FLAG_DID_OUTPUT)) {
            voiceState->flags = 0;
        } else if (state == AL_PLAYING || voiceState->timeRemaining == 0.0f) {
            voiceState->flags |= AUDIO_VOICE_FLAG_DID_OUTPUT;
        }
    }
}

static int audioIsSoundClipLooped(int soundClipId) {
    if (soundClipId < 0 || soundClipId >= sSoundClipArray->soundCount) {
        return 0;
    }

    ALSound* soundClip = sSoundClipArray->sounds[soundClipId];
    ALWaveTable* wavetable = soundClip->wavetable;

    if (wavetable) {
        if (wavetable->type == AL_ADPCM_WAVE) {
            return wavetable->waveInfo.adpcmWave.loop != NULL;
        } else if (wavetable->type == AL_RAW16_WAVE) {
            return wavetable->waveInfo.rawWave.loop != NULL;
        }
    }

    return 0;
}

static float audioSoundLength(int soundClipId, float pitch) {
    if (audioIsSoundClipLooped(soundClipId)) {
        return -1.0f;
    }

    ALSound* soundClip = sSoundClipArray->sounds[soundClipId];
    ALWaveTable* wavetable = soundClip->wavetable;
    int sampleCount = 0;

    if (wavetable) {
        if (wavetable->type == AL_ADPCM_WAVE) {
            // N64 ADPCM encodes groups of 16 samples into 9-byte blocks
            sampleCount = wavetable->len * 16 / 9;
        } else if (wavetable->type == AL_RAW16_WAVE) {
            // 16-bit samples
            sampleCount = wavetable->len / 2;
        }
    }

    return sampleCount * (1.0f / AUDIO_OUTPUT_HZ) / pitch;
}

VoiceId audioPlaySound(int soundClipId, float volume, float pitch, float pan, float echo) {
    if (soundClipId < 0 || soundClipId >= sSoundClipArray->soundCount) {
        return VOICE_ID_NONE;
    }

    ALSound* soundClip = sSoundClipArray->sounds[soundClipId];
    ALSndId voiceId = alSndpAllocate(&sSoundPlayer, soundClip);
    if (voiceId == VOICE_ID_NONE) {
        return voiceId;
    }

    struct VoiceState* voiceState = &sVoiceStates[voiceId];
    voiceState->timeRemaining = audioSoundLength(soundClipId, pitch);
    voiceState->pitch = 0.0f;
    voiceState->volume = 0;
    voiceState->flags = AUDIO_VOICE_FLAG_PLAYING;

    audioSetSoundParams(voiceId, volume, pitch, pan, echo);
    alSndpPlay(&sSoundPlayer);

    return voiceId;
}

void audioSetSoundParams(VoiceId voiceId, float volume, float pitch, float pan, float echo) {
    struct VoiceState* voiceState = &sVoiceStates[voiceId];
    alSndpSetSound(&sSoundPlayer, voiceId);

    if (volume >= 0.0f) {
        voiceState->volume = 32767 * volume;

        if (!(voiceState->flags & AUDIO_VOICE_FLAG_PAUSED)) {
            alSndpSetVol(&sSoundPlayer, voiceState->volume);
        }
    }

    if (pitch >= 0.0f) {
        voiceState->pitch = pitch;

        if (!(voiceState->flags & AUDIO_VOICE_FLAG_PAUSED)) {
            alSndpSetPitch(&sSoundPlayer, pitch);
        }
    }

    if (pan >= 0.0f) {
        alSndpSetPan(&sSoundPlayer, 127 * pan);
    }

    if (echo >= 0.0f) {
        alSndpSetFXMix(&sSoundPlayer, 127 * echo);
    }
}

int audioIsSoundPlaying(VoiceId voiceId) {
    return sVoiceStates[voiceId].flags & AUDIO_VOICE_FLAG_PLAYING;
}

int audioIsSoundLooped(VoiceId voiceId) {
    return sVoiceStates[voiceId].timeRemaining == -1.0f;
}

void audioPauseSound(VoiceId voiceId) {
    // No good way to pause or start sounds partway through, so play at pitch ~0
    // This doesn't affect the end time, so stopping is handled by us, not the library
    sVoiceStates[voiceId].flags |= AUDIO_VOICE_FLAG_PAUSED;

    alSndpSetSound(&sSoundPlayer, voiceId);
    alSndpSetVol(&sSoundPlayer, 0);
    alSndpSetPitch(&sSoundPlayer, 0.0f);
}

int audioIsSoundPaused(VoiceId voiceId) {
    return sVoiceStates[voiceId].flags & AUDIO_VOICE_FLAG_PAUSED;
}

void audioResumeSound(VoiceId voiceId) {
    struct VoiceState* voiceState = &sVoiceStates[voiceId];
    voiceState->flags &= ~AUDIO_VOICE_FLAG_PAUSED;

    alSndpSetSound(&sSoundPlayer, voiceId);
    alSndpSetVol(&sSoundPlayer, voiceState->volume);
    alSndpSetPitch(&sSoundPlayer, voiceState->pitch);
}

void audioStopSound(VoiceId voiceId) {
    struct VoiceState* voiceState = &sVoiceStates[voiceId];
    voiceState->timeRemaining = 0.0f;
    voiceState->flags &= ~AUDIO_VOICE_FLAG_PAUSED;

    alSndpSetSound(&sSoundPlayer, voiceId);
    alSndpStop(&sSoundPlayer);
}

void audioReleaseVoice(VoiceId voiceId) {
    alSndpDeallocate(&sSoundPlayer, voiceId);
}

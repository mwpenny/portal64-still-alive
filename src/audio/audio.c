
#include "audio.h"
#include "util/rom.h"

#include "defs.h"

/**** audio globals ****/
u8* gAudioHeapBuffer;
ALHeap             gAudioHeap;

void initAudio(int fps) 
{
    ALSynConfig   c;
    amConfig      amc;
    
    alHeapInit(&gAudioHeap, gAudioHeapBuffer, AUDIO_HEAP_SIZE);    

    /*
     * Create the Audio Manager
     */
    c.maxVVoices = MAX_VOICES;
    c.maxPVoices = MAX_VOICES;
    c.maxUpdates = MAX_UPDATES;
    c.dmaproc    = 0;                  /* audio mgr will fill this in */
    c.fxType	 = AL_FX_SMALLROOM;
    c.outputRate = 0;                  /* audio mgr will fill this in */
    c.heap       = &gAudioHeap;
    
    amc.outputRate = 44100;
    amc.framesPerField = NUM_FIELDS;
    amc.maxACMDSize = MAX_RSP_CMDS;
 
    amCreateAudioMgr(&c, AUDIO_PRIORITY, &amc, fps);
}

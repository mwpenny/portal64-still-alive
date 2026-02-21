#ifndef __AUDIO_H__
#define __AUDIO_H__

#define AUDIO_OUTPUT_HZ 22050

struct SoundArray {
    u32 soundCount;
    ALSound* sounds[];
};

void* audioInit(void* memoryEnd, int maxVoices);

#endif

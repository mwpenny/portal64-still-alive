
#ifndef _AUDIO_SOUNDARRAY_H
#define _AUDIO_SOUNDARRAY_H

#include <ultra64.h>

struct SoundArray {
    u32 soundCount;
    ALSound* sounds[];
};

void soundArrayInit(struct SoundArray* soundArray, void* tbl);

#endif
#ifndef __CONTROLS_RUMBLE_PAK_CLIP_H__
#define __CONTROLS_RUMBLE_PAK_CLIP_H__

#define RUMBLE_ID_NONE -1

typedef short RumbleID;

struct RumblePakWave {
    // 2 bits per sample
    unsigned char* samples;
    short sampleCount;
    short samplesPerSecond;
};

void rumblePakClipInit();
void rumblePakClipUpdate();
void rumblePakClipSetPaused(int paused);

RumbleID rumblePakClipPlay(struct RumblePakWave* wave);
int rumblePakClipIsActive(RumbleID clip);
void rumblePakClipStop(RumbleID clip);

#endif

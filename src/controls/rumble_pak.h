#ifndef __CONTROLS__RUMBLE_PAK_H__
#define __CONTROLS__RUMBLE_PAK_H__

struct RumblePakWave {
    // 2 bits per sample
    unsigned char* samples;
    short sampleCount;
    // 10.6 fixed point number ticks happens at 60 times per second (50 for pal)
    short samplesPerTick;
};

typedef short RumbleID;

struct RumblePakClip {
    struct RumblePakWave* wave;
    struct RumblePakClip* next;
    // 10.6 fixed point number
    short currentSample;
    RumbleID rumbleId;
};

void rumblePakClipInit();
RumbleID rumblePakClipPlay(struct RumblePakWave* wave);
int rumblePakClipIsActive(RumbleID clip);
void rumblePakClipStop(RumbleID clip);
void rumblePakSetPaused(int paused);

int rumblePakCalculateState();

#endif
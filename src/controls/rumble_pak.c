#include "rumble_pak.h"

#include <ultra64.h>
#include "../util/time.h"

#define MAX_ACTIVE_RUMBLE   4

struct RumblePakClip gClips[MAX_ACTIVE_RUMBLE];
struct RumblePakClip* gFirstActiveClip = NULL;
struct RumblePakClip* gFirstIdleClip = NULL;
RumbleID gNextRumbleId = 1;
u8 gRumbleIsPaused = 0;

void rumblePakClipInit() {
    struct RumblePakClip* prev = NULL;
    
    for (int i = 0; i < MAX_ACTIVE_RUMBLE; ++i) {
        struct RumblePakClip* curr = &gClips[i];

        if (prev) {
            prev->next = curr;
        } else {
            gFirstIdleClip = curr;
        }

        curr->wave = NULL;
        curr->currentSample = 0;
        curr->rumbleId = 0;

        prev = curr;
    }

    prev->next = NULL;
    gFirstActiveClip = NULL;
}

RumbleID rumblePakClipPlay(struct RumblePakWave* wave) {
    if (!gFirstIdleClip) {
        return 0;
    }

    struct RumblePakClip* clip = gFirstIdleClip;

    gFirstIdleClip = gFirstIdleClip->next;

    clip->next = gFirstActiveClip;
    gFirstActiveClip = clip;
    clip->currentSample = 0;
    clip->wave = wave;
    clip->rumbleId = gNextRumbleId;
    ++gNextRumbleId;

    return clip->rumbleId;
}

int rumblePakClipIsActive(RumbleID clip) {
    struct RumblePakClip* curr = gFirstActiveClip;

    while (curr) {
        if (curr->rumbleId == clip) {
            return TRUE;
        }

        curr = curr->next;
    }

    return FALSE;
}

void rumblePakClipStop(RumbleID clipId) {
    struct RumblePakClip* clip = gFirstActiveClip;

    while (clip) {
        if (clip->rumbleId == clipId) {
            break;
        }

        clip = clip->next;
    }

    if (!clip) {
        return;
    }

    struct RumblePakClip* curr = gFirstActiveClip;
    struct RumblePakClip* prev = NULL;

    while (curr) { 
        if (curr == clip) {
            if (prev) {
                prev->next = curr->next;
            } else {
                gFirstActiveClip = curr->next;
            }

            curr->wave = NULL;
            curr->next = NULL;

            curr->next = gFirstIdleClip;
            gFirstIdleClip = curr;
            return;
        }

        curr = curr->next;
    }
    
}

void rumblePakSetPaused(int paused) {
    gRumbleIsPaused = paused;
}

int gRumbleCurrentBit = 0;

int rumblePakCalculateState() {
    if (gRumbleIsPaused) {
        return 0;
    }

    int amplitude = 0;

    struct RumblePakClip* curr = gFirstActiveClip;
    struct RumblePakClip* prev = NULL;

    while (curr) {
        int sampleAsInt = curr->currentSample >> 6;

        if (sampleAsInt >= curr->wave->sampleCount) {
            // clip is done, remove it
            curr->wave = NULL;
            
            if (prev) {
                prev->next = curr->next;
            } else {
                gFirstActiveClip = curr->next;
            }

            struct RumblePakClip* next = curr->next;
            curr->next = gFirstIdleClip;
            gFirstIdleClip = curr;
            curr = next;
        } else {
            int byteIndex = sampleAsInt >> 2;
            int byteOffset = sampleAsInt & 0x3;

            u8 byte = curr->wave->samples[byteIndex];

            amplitude += (byte >> (6 - byteOffset * 2)) & 0x3;

            curr->currentSample += curr->wave->samplesPerTick;

            prev = curr;
            curr = curr->next;
        }
    } 

    if (amplitude > 3) {
        amplitude = 3;
    }

    int result = amplitude == 3 || amplitude > gRumbleCurrentBit;
    gRumbleCurrentBit = (gRumbleCurrentBit + 1) & 0x3;
    return result;
}
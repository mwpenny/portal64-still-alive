#include "rumble_pak_clip.h"

#include "controller_actions.h"
#include "system/controller.h"
#include "util/frame_time.h"
#include "util/memory.h"

#include <stddef.h>

#define RUMBLE_MAX_CLIPS 4

struct RumblePakClip {
    struct RumblePakClip* next;

    struct RumblePakWave* wave;
    float currentSample;
    float sampleIncrement;
    RumbleID rumbleId;
};

static struct RumblePakClip sClips[RUMBLE_MAX_CLIPS];
static struct RumblePakClip* sFirstActiveClip = NULL;
static struct RumblePakClip* sFirstIdleClip = NULL;

static RumbleID sNextRumbleId = 0;
static uint8_t sRumbleClipsPaused = 0;
static int sRumbleCurrentStep = 0;

static void stopClip(struct RumblePakClip* clip, struct RumblePakClip* prev) {
    clip->wave = NULL;
    clip->currentSample = 0.0f;
    clip->sampleIncrement = 0.0f;
    clip->rumbleId = RUMBLE_ID_NONE;

    if (prev) {
        prev->next = clip->next;
    } else {
        sFirstActiveClip = clip->next;
    }

    clip->next = sFirstIdleClip;
    sFirstIdleClip = clip;
}

static int rumblePakCalculateState() {
    if (sRumbleClipsPaused) {
        return 0;
    }

    int amplitude = 0;

    struct RumblePakClip* curr = sFirstActiveClip;
    struct RumblePakClip* prev = NULL;

    while (curr) {
        int sampleAsInt = curr->currentSample;

        if (sampleAsInt >= curr->wave->sampleCount) {
            // Clip is done, remove it
            struct RumblePakClip* next = curr->next;
            stopClip(curr, prev);
            curr = next;
        } else {
            // 4 samples per byte
            int byteIndex = sampleAsInt >> 2;
            int byteOffset = sampleAsInt & 0x3;

            // 2 bits per sample
            uint8_t byte = curr->wave->samples[byteIndex];
            amplitude += (byte >> (6 - (byteOffset * 2))) & 0x3;

            curr->currentSample += curr->sampleIncrement;

            prev = curr;
            curr = curr->next;
        }
    }

    // Multiple waves could be playing together and stack
    if (amplitude > 3) {
        amplitude = 3;
    }

    // Always off if 0 amplitude
    // Always on if max amplitude (3)
    // Otherwise stay on amplitude/4 of the time
    int result = amplitude == 3 || amplitude > sRumbleCurrentStep;
    sRumbleCurrentStep = (sRumbleCurrentStep + 1) & 0x3;
    return result;
}

void rumblePakClipInit() {
    struct RumblePakClip* prev = NULL;

    for (int i = 0; i < RUMBLE_MAX_CLIPS; ++i) {
        struct RumblePakClip* curr = &sClips[i];

        curr->wave = NULL;
        curr->currentSample = 0.0f;
        curr->sampleIncrement = 0.0f;
        curr->rumbleId = RUMBLE_ID_NONE;

        if (prev) {
            prev->next = curr;
        } else {
            sFirstIdleClip = curr;
        }

        prev = curr;
    }

    prev->next = NULL;
    sFirstActiveClip = NULL;
    sRumbleClipsPaused = 0;
}

void rumblePakClipUpdate() {
    int state = rumblePakCalculateState();

    for (int controllerIndex = 0; controllerIndex < MAX_BINDABLE_CONTROLLERS; ++controllerIndex) {
        if (controllerActionUsesController(controllerIndex)) {
            controllerSetRumble(controllerIndex, state);
        }
    }
}

void rumblePakClipSetPaused(int paused) {
    sRumbleClipsPaused = paused;
}

RumbleID rumblePakClipPlay(struct RumblePakWave* wave) {
    struct RumblePakClip* clip = sFirstIdleClip;
    if (!clip) {
        return RUMBLE_ID_NONE;
    }

    clip->wave = wave;
    clip->currentSample = 0.0f;
    clip->sampleIncrement = wave->samplesPerSecond * FIXED_DELTA_TIME;
    clip->rumbleId = ++sNextRumbleId & 0xFFF;

    sFirstIdleClip = clip->next;
    clip->next = sFirstActiveClip;
    sFirstActiveClip = clip;

    return clip->rumbleId;
}

int rumblePakClipIsActive(RumbleID clipId) {
    for (struct RumblePakClip* curr = sFirstActiveClip; curr; curr = curr->next) {
        if (curr->rumbleId == clipId) {
            return 1;
        }
    }

    return 0;
}

void rumblePakClipStop(RumbleID clipId) {
    struct RumblePakClip* prev = NULL;

    for (struct RumblePakClip* curr = sFirstActiveClip; curr; curr = curr->next) {
        if (curr->rumbleId == clipId) {
            stopClip(curr, prev);
            return;
        }

        prev = curr;
    }
}

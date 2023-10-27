#include "./player_rumble_clips.h"

#include "../physics/rigid_body.h"

unsigned char gPlayerDieRumbleData[] = {
    0xFF, 0xE9, 0x9E, 0xFF, 0xFF, 0xE9
};

struct RumblePakWave gPlayerDieRumbleWave = {
    .samples = gPlayerDieRumbleData,
    .sampleCount = 24,
    .samplesPerTick = 1 << 5,
};

unsigned char gPlayerClosePortalRumbleData[] = {
    0xAA, 0x99,
};

struct RumblePakWave gPlayerClosePortalRumble = {
    .samples = gPlayerClosePortalRumbleData,
    .sampleCount = 8,
    .samplesPerTick = 1 << 5,
};

unsigned char gPlayerLandSoftData[] = {
    0xFE, 0x90,
};

unsigned char gPlayerLandMediumData[] = {
    0xFF, 0xFE, 0x90,
};

unsigned char gPlayerLandHardData[] = {
    0xFF, 0xFF, 0xFF, 0xE9
};

#define PLAYER_LANDING_CLIP_COUNT   3

struct RumblePakWave gPlayerLandWaves[PLAYER_LANDING_CLIP_COUNT] = {
    {
        .samples = gPlayerLandHardData,
        .sampleCount = 16,
        .samplesPerTick = 1 << 5,
    },
    {
        .samples = gPlayerLandMediumData,
        .sampleCount = 10,
        .samplesPerTick = 1 << 5,
    },
    {
        .samples = gPlayerLandSoftData,
        .sampleCount = 6,
        .samplesPerTick = 1 << 5,
    },
};

float gLandRumbleThresholds[PLAYER_LANDING_CLIP_COUNT] = {
    0.80f * MAX_PORTAL_SPEED,
    0.60f * MAX_PORTAL_SPEED,
    0.40f * MAX_PORTAL_SPEED,
};

void playerHandleLandingRumble(float velocityChange) {
    for (int i = 0; i < PLAYER_LANDING_CLIP_COUNT; ++i) {
        if (velocityChange > gLandRumbleThresholds[i]) {
            rumblePakClipPlay(&gPlayerLandWaves[i]);
            break;
        }
    }
}
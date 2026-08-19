#include "player_rumble_clips.h"

#include "physics/rigid_body.h"

static unsigned char sPlayerDamageRumbleData[] = {
    0xFA, 0x55
};
struct RumblePakWave gPlayerDamageRumbleWave = {
    .samples = sPlayerDamageRumbleData,
    .sampleCount = 8,
    .samplesPerSecond = 15,
};

static unsigned char sPlayerDieRumbleData[] = {
    0xFF, 0xE9, 0x9E, 0xFF, 0xFF, 0xE9
};
struct RumblePakWave gPlayerDieRumbleWave = {
    .samples = sPlayerDieRumbleData,
    .sampleCount = 24,
    .samplesPerSecond = 15,
};

static unsigned char sPlayerClosePortalRumbleData[] = {
    0xAA, 0x99,
};
struct RumblePakWave gPlayerClosePortalRumble = {
    .samples = sPlayerClosePortalRumbleData,
    .sampleCount = 8,
    .samplesPerSecond = 15,
};

static unsigned char sPlayerLandSoftData[] = {
    0xFE, 0x90,
};
static unsigned char sPlayerLandMediumData[] = {
    0xFF, 0xFE, 0x90,
};
static unsigned char sPlayerLandHardData[] = {
    0xFF, 0xFF, 0xFF, 0xE9
};

#define PLAYER_LANDING_CLIP_COUNT   3

static struct RumblePakWave sPlayerLandWaves[PLAYER_LANDING_CLIP_COUNT] = {
    {
        .samples = sPlayerLandHardData,
        .sampleCount = 16,
        .samplesPerSecond = 15,
    },
    {
        .samples = sPlayerLandMediumData,
        .sampleCount = 10,
        .samplesPerSecond = 15,
    },
    {
        .samples = sPlayerLandSoftData,
        .sampleCount = 6,
        .samplesPerSecond = 15,
    },
};

static float sLandRumbleThresholds[PLAYER_LANDING_CLIP_COUNT] = {
    0.80f * MAX_PORTAL_SPEED,
    0.60f * MAX_PORTAL_SPEED,
    0.40f * MAX_PORTAL_SPEED,
};

void playerHandleLandingRumble(float velocityChange) {
    for (int i = 0; i < PLAYER_LANDING_CLIP_COUNT; ++i) {
        if (velocityChange > sLandRumbleThresholds[i]) {
            rumblePakClipPlay(&sPlayerLandWaves[i]);
            break;
        }
    }
}

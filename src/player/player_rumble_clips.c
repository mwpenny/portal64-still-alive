#include "./player_rumble_clips.h"

unsigned char gPlayerPassPortalData[] = {
    0xFE, 0x09,
};

struct RumblePakWave gPlayerPassPortalWave = {
    .samples = gPlayerPassPortalData,
    .sampleCount = 6,
    .samplesPerTick = 1 << 6,
};

unsigned char gPlayerDieRumbleData[] = {
    0xFF, 0xE9, 0x9E, 0xFF, 0xFF, 0xE9
};

struct RumblePakWave gPlayerDieRumbleWave = {
    .samples = gPlayerDieRumbleData,
    .sampleCount = 24,
    .samplesPerTick = 1 << 5,
};
#ifndef __PLAYER_PLAYER_RUMBLE_CLIPS_H__
#define __PLAYER_PLAYER_RUMBLE_CLIPS_H__

#include "../controls/rumble_pak.h"

extern struct RumblePakWave gPlayerDieRumbleWave;

extern struct RumblePakWave gPlayerClosePortalRumble;

void playerHandleLandingRumble(float velocityChange);

#endif
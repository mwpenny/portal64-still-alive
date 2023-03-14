#ifndef __LEVELS_SAVEFILE_H__
#define __LEVELS_SAVEFILE_H__

enum SavefileFlags {
    SavefileFlagsFirstPortalGun = (1 << 0),
    SavefileFlagsSecondPortalGun = (1 << 1),
};

void savefileNew();

void savefileSetFlags(enum SavefileFlags flags);

void savefileUnsetFlags(enum SavefileFlags flags);

int savefileReadFlags(enum SavefileFlags flags);

#endif
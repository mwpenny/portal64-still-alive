#include "savefile.h"

#include "../util/memory.h"

struct SaveFile {
    short flags;
    char data[128];
};

struct SaveFile gCurrentSave;

void savefileNew() {
    zeroMemory(&gCurrentSave, sizeof(struct SaveFile));
}

void savefileSetFlags(enum SavefileFlags flags) {
    gCurrentSave.flags |= flags;
}

int savefileReadFlags(enum SavefileFlags flags) {
    return gCurrentSave.flags & flags;
}
#include "system/cartridge.h"

// TODO

void cartridgeInit() {
}

void romCopy(const void* romAddr, void* ramAddr, const int size) {
}

void romCopyAsync(const void* romAddr, void* ramAddr, const int size) {
}

void romCopyAsyncDrain() {
}

void sramWrite(void* sramAddr, const void* ramAddr, const int size) {
}

int sramRead(const void* sramAddr, void* ramAddr, const int size) {
    return 0;
}


#include "signals.h"

#include "../util/memory.h"

unsigned long long* gSignals;
unsigned long long* gDefaultSignals;
unsigned gSignalCount;

#define DETERMINE_BIN_AND_MASK(bin, mask, signalIndex) do { bin = (signalIndex) >> 6; mask = 1LL << ((signalIndex) & 63); } while (0)

void signalsInit(unsigned signalCount) {
    if (!signalCount) {
        return;
    }

    int binCount = (signalCount + 63) >> 6;
    gSignals = malloc(sizeof(unsigned long long) * binCount);
    gDefaultSignals = malloc(sizeof(unsigned long long) * binCount);
    gSignalCount = signalCount;

    for (int i = 0; i < binCount; ++i) {
        gDefaultSignals[i] = 0;
    }
}

void signalsReset() {
    int binCount = (gSignalCount + 63) >> 6;

    for (int i = 0; i < binCount; ++i) {
        gSignals[i] = gDefaultSignals[i];
    }
}

int signalsRead(unsigned signalIndex) {
    unsigned bin;
    unsigned long long mask;

    DETERMINE_BIN_AND_MASK(bin, mask, signalIndex);

    if (bin >= gSignalCount) {
        return 0;
    }

    return (gSignals[bin] & mask) != 0;
}

void signalsSend(unsigned signalIndex) {
    unsigned bin;
    unsigned long long mask;

    DETERMINE_BIN_AND_MASK(bin, mask, signalIndex);

    if (bin >= gSignalCount) {
        return;
    }

    gSignals[bin] = (gSignals[bin] & ~mask) | (gDefaultSignals[bin] ^ mask);
}

void signalsSetDefault(unsigned signalIndex, int value) {
    unsigned bin;
    unsigned long long mask;

    DETERMINE_BIN_AND_MASK(bin, mask, signalIndex);

    if (bin >= gSignalCount) {
        return;
    }

    gDefaultSignals[bin] = (gDefaultSignals[bin] & ~mask) | (value ? mask : 0);
}

#include "signals.h"

#include "../util/memory.h"

unsigned long long* gSignals;
unsigned long long* gPrevSignals;
unsigned long long* gDefaultSignals;
unsigned gSignalCount;

#define DETERMINE_BIN_AND_MASK(bin, mask, signalIndex) do { bin = (signalIndex) >> 6; mask = 1LL << ((signalIndex) & 63); } while (0)

void signalsInit(unsigned signalCount) {
    if (!signalCount) {
        return;
    }

    int binCount = SIGNAL_BIN_COUNT(signalCount);
    gSignals = malloc(sizeof(unsigned long long) * binCount);
    gPrevSignals = malloc(sizeof(unsigned long long) * binCount);
    gDefaultSignals = malloc(sizeof(unsigned long long) * binCount);
    gSignalCount = signalCount;

    for (int i = 0; i < binCount; ++i) {
        gDefaultSignals[i] = 0;
        gPrevSignals[i] = 0;
        gSignals[i] = 0;
    }
}

void signalsReset() {
    int binCount = (gSignalCount + 63) >> 6;

    for (int i = 0; i < binCount; ++i) {
        gPrevSignals[i] = gSignals[i];
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

int signalsReadPrevious(unsigned signalIndex) {
    unsigned bin;
    unsigned long long mask;

    DETERMINE_BIN_AND_MASK(bin, mask, signalIndex);

    if (bin >= gSignalCount) {
        return 0;
    }

    return (gPrevSignals[bin] & mask) != 0;
}

int signalCount() {
    return gSignalCount;
}

void signalsSend(unsigned signalIndex) {
    unsigned bin;
    unsigned long long mask;

    DETERMINE_BIN_AND_MASK(bin, mask, signalIndex);

    if (bin >= gSignalCount) {
        return;
    }

    gSignals[bin] = (gSignals[bin] & ~mask) | ((gDefaultSignals[bin] ^ mask) & mask);
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

void signalsEvaluateSignal(struct SignalOperator* operator) {
    switch (operator->type) {
        case SignalOperatorTypeAnd:
            if (signalsRead(operator->inputSignals[0]) && signalsRead(operator->inputSignals[1])) {
                signalsSend(operator->outputSignal);
            }
            break;
        case SignalOperatorTypeOr:
            if (signalsRead(operator->inputSignals[0]) || signalsRead(operator->inputSignals[1])) {
                signalsSend(operator->outputSignal);
            }
            break;
        case SignalOperatorTypeNot:
            if (!signalsRead(operator->inputSignals[0])) {
                signalsSend(operator->outputSignal);
            }
            break;
        case SignalOperatorTypeTimer:
            break;
    }
}

void signalsEvaluateSignals(struct SignalOperator* operator, unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        signalsEvaluateSignal(&operator[i]);
    }
}

void signalsSerializeRW(struct Serializer* serializer, SerializeAction action) {
    int binCount = SIGNAL_BIN_COUNT(gSignalCount);
    action(serializer, gSignals, sizeof(unsigned long long) * binCount);
    action(serializer, gDefaultSignals, sizeof(unsigned long long) * binCount);
}
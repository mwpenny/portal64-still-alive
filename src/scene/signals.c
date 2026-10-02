#include "signals.h"

#include "util/memory.h"

#define SIGNAL_BIN_COUNT(signalCount) (((signalCount) + 63) >> 6)
#define SIGNAL_BIN_AND_MASK(bin, mask, signalIndex) do { bin = (signalIndex) >> 6; mask = 1LL << ((signalIndex) & 63); } while (0)

static unsigned int sBinCount;
static unsigned long long* sDefaultSignals;
static unsigned long long* sPrevSignals;
static unsigned long long* sSignals;
static unsigned long long* sQueuedSignals;

void signalsInit(unsigned int signalCount) {
    if (!signalCount) {
        signalCount = 1;
    }

    sBinCount = SIGNAL_BIN_COUNT(signalCount);
    sDefaultSignals = malloc(sizeof(unsigned long long) * sBinCount);
    sPrevSignals = malloc(sizeof(unsigned long long) * sBinCount);
    sSignals = malloc(sizeof(unsigned long long) * sBinCount);
    sQueuedSignals = malloc(sizeof(unsigned long long) * sBinCount);

    for (int i = 0; i < sBinCount; ++i) {
        sDefaultSignals[i] = 0;
        sPrevSignals[i] = 0;
        sSignals[i] = 0;
        sQueuedSignals[i] = 0;
    }
}

void signalsReset() {
    for (unsigned int i = 0; i < sBinCount; ++i) {
        sPrevSignals[i] = sSignals[i];
        sSignals[i] = sDefaultSignals[i] ^ sQueuedSignals[i];
        sQueuedSignals[i] = 0;
    }
}

int signalsRead(unsigned int signalIndex) {
    unsigned int bin;
    unsigned long long mask;

    SIGNAL_BIN_AND_MASK(bin, mask, signalIndex);

    if (bin >= sBinCount) {
        return 0;
    }

    return (sSignals[bin] & mask) != 0;
}

int signalsReadPrevious(unsigned int signalIndex) {
    unsigned int bin;
    unsigned long long mask;

    SIGNAL_BIN_AND_MASK(bin, mask, signalIndex);

    if (bin >= sBinCount) {
        return 0;
    }

    return (sPrevSignals[bin] & mask) != 0;
}

void signalsSend(unsigned int signalIndex) {
    unsigned int bin;
    unsigned long long mask;

    SIGNAL_BIN_AND_MASK(bin, mask, signalIndex);

    if (bin >= sBinCount) {
        return;
    }

    sSignals[bin] = (sSignals[bin] & ~mask) | ((sDefaultSignals[bin] ^ mask) & mask);
}

void signalsQueue(unsigned int signalIndex) {
    unsigned int bin;
    unsigned long long mask;

    SIGNAL_BIN_AND_MASK(bin, mask, signalIndex);

    if (bin >= sBinCount) {
        return;
    }

    sQueuedSignals[bin] |= mask;
}

void signalsSetDefault(unsigned int signalIndex, int value) {
    unsigned int bin;
    unsigned long long mask;

    SIGNAL_BIN_AND_MASK(bin, mask, signalIndex);

    if (bin >= sBinCount) {
        return;
    }

    sDefaultSignals[bin] = (sDefaultSignals[bin] & ~mask) | (value ? mask : 0);
}

static void evaluateOperator(struct SignalOperator* operator) {
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
    }
}

void signalsEvaluateOperators(struct SignalOperator* operator, unsigned int count) {
    for (unsigned int i = 0; i < count; ++i) {
        evaluateOperator(&operator[i]);
    }
}

void signalsSerializeRW(struct Serializer* serializer, SerializeAction action) {
    action(serializer, sSignals, sizeof(unsigned long long) * sBinCount);
    action(serializer, sDefaultSignals, sizeof(unsigned long long) * sBinCount);
    action(serializer, sQueuedSignals, sizeof(unsigned long long) * sBinCount);
}

#ifndef __SCENE_SIGNALS_H__
#define __SCENE_SIGNALS_H__

#include "../savefile/serializer.h"

#define SIGNAL_BIN_COUNT(signalCount) (((signalCount) + 63) >> 6)

enum SignalOperatorType {
    SignalOperatorTypeAnd,
    SignalOperatorTypeOr,
    SignalOperatorTypeNot,
    SignalOperatorTypeTimer,
};

struct SignalOperator {
    unsigned char type;
    unsigned char outputSignal;
    unsigned char inputSignals[2];
    union {
        char additionalInputs[2];
        short duration;
    } data;
};

void signalsInit(unsigned signalCount);
void signalsReset();
int signalsRead(unsigned signalIndex);
int signalsReadPrevious(unsigned signalIndex);
int signalCount();
void signalsSend(unsigned signalIndex);
void signalsSetDefault(unsigned signalIndex, int value);
void signalsEvaluateSignals(struct SignalOperator* operator, unsigned count);

void signalsSerializeRW(struct Serializer* serializer, SerializeAction action);

#endif
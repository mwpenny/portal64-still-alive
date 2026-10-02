#ifndef __SCENE_SIGNALS_H__
#define __SCENE_SIGNALS_H__

#include "savefile/serializer.h"

enum SignalOperatorType {
    SignalOperatorTypeAnd,
    SignalOperatorTypeOr,
    SignalOperatorTypeNot,
};

struct SignalOperator {
    unsigned char type;
    unsigned char outputSignal;
    unsigned char inputSignals[2];
};

void signalsInit(unsigned int signalCount);
void signalsReset();

int signalsRead(unsigned int signalIndex);
int signalsReadPrevious(unsigned int signalIndex);
void signalsSend(unsigned int signalIndex);
void signalsQueue(unsigned int signalIndex);
void signalsSetDefault(unsigned int signalIndex, int value);
void signalsEvaluateOperators(struct SignalOperator* operator, unsigned int count);

void signalsSerializeRW(struct Serializer* serializer, SerializeAction action);

#endif

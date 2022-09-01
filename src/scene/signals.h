#ifndef __SCENE_SIGNALS_H__
#define __SCENE_SIGNALS_H__

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
void signalsSend(unsigned signalIndex);
void signalsSetDefault(unsigned signalIndex, int value);
void signalsEvaluateSignals(struct SignalOperator* operator, unsigned count);

#endif
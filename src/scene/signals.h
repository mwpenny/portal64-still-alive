#ifndef __SCENE_SIGNALS_H__
#define __SCENE_SIGNALS_H__

void signalsInit(unsigned signalCount);
void signalsReset();
int signalsRead(unsigned signalIndex);
void signalsSend(unsigned signalIndex);
void signalsSetDefault(unsigned signalIndex, int value);

#endif
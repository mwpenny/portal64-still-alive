
#include "mathf.h"

unsigned int gRandomSeed = 1;

#define MAX_INT_VALUE   0x7fff

int randomInt() {
    gRandomSeed = gRandomSeed * 22695477 + 1;
    return (gRandomSeed >> 16) & MAX_INT_VALUE;
}

int randomInRange(int min, int maxPlusOne) {
    return randomInt() * (maxPlusOne - min) / (MAX_INT_VALUE + 1) + min;
}

float randomInRangef(float min, float maxPlusOne) {
    return randomInt() * (maxPlusOne - min) / (MAX_INT_VALUE + 1) + min;
}

float fabsf(float input) {
    if (input < 0) {
        return -input;
    } else {
        return input;
    }
}

float mathfLerp(float from, float to, float t) {
    return from * (1.0f - t) + to * t;
}

float mathfInvLerp(float from, float to, float value) {
    return (value - from) / (to - from);
}

float mathfMoveTowards(float from, float to, float maxMove) {
    float offset = to - from;
    if (fabsf(offset) <= maxMove) {
        return to;
    } else {
        return signf(offset) * maxMove + from;
    }
}

float mathfMod(float input, float divisor) {
    float floorDivide = floorf(input / divisor);
    return input - floorDivide * divisor;
}

float floorf(float input) {
    int asint = (int)input;
    if (input >= 0 || input == asint) {
        return asint;
    } else {
        return asint - 1;
    }
}

float ceilf(float input) {
    int asint = (int)input;
    if (input <= 0 || input == asint) {
        return asint;
    } else {
        return asint + 1;
    }
}

float mathfBounceBackLerp(float t) {
    return -t + t * t;
}

float mathfRandomFloat() {
    return (float)randomInt() / (float)0x7fff;
}

float clampf(float input, float min, float max) {
    if (input < min) {
        return min;
    }

    if (input > max) {
        return max;
    }

    return input;
}

float signf(float input) {
    if (input > 0.0f) {
        return 1.0f;
    } else if (input < 0.0f) {
        return -1.0f;
    } else {
        return 0.0f;
    }
}


int sign(int input) {
    if (input > 0) {
        return 1;
    } else if (input < 0) {
        return -1;
    } else {
        return 0;
    }
}

int abs(int input) {
    if (input < 0) {
        return -input;
    }
    return input;
}

float minf(float a, float b) {
    return a < b ? a : b;
}

float maxf(float a, float b) {
    return a > b ? a : b;
}

char floatTos8norm(float input) {
    int result = (int)(input * 127.0f);

    if (result > 127) {
        return 127;
    } else if (result < -127) {
        return -127;
    } else {
        return (char)result;
    }
}

float safeInvert(float input) {
    if (input == 0.0f) {
        return 0.0f;
    }

    return 1.0f / input;
}
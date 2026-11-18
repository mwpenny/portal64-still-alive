#ifndef _MATH_MATHF_H_
#define _MATH_MATHF_H_

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int randomInt();
int randomInRange(int min, int maxPlusOne);
float mathfRandomFloat();
float randomInRangef(float min, float max);

float mathfLerp(float from, float to, float t);
float mathfInvLerp(float from, float to, float value);
float mathfMoveTowards(float from, float to, float maxMove);
float mathfBounceBackLerp(float t);

int abs(int input);
float fabsf(float in);
float floorf(float in);
float ceilf(float in);
int sign(int input);
float signf(float input);
float mathfMod(float input, float divisor);
float clampf(float input, float min, float max);
int min(int a, int b);
int max(int a, int b);
float minf(float a, float b);
float maxf(float a, float b);

float sqrtf(float in);
float powf(float base, float exp);
float safeInvert(float input);

float cosf(float in);
float sinf(float in);

char floatTos8norm(float input);


#endif

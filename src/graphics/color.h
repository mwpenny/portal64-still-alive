#ifndef _MATH_COLOR_H
#define _MATH_COLOR_H

struct Coloru8 {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

extern struct Coloru8 gColorBlack;
extern struct Coloru8 gColorWhite;
extern struct Coloru8 gHalfTransparentBlack;
extern struct Coloru8 gHalfTransparentWhite;

void colorU8Lerp(struct Coloru8* from, struct Coloru8* to, float lerp, struct Coloru8* output);
void colorU8Mul(struct Coloru8* a, struct Coloru8* b, struct Coloru8* out);

#endif
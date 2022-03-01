#ifndef __SCREEN_CLIPPER_H__
#define __SCREEN_CLIPPER_H__

#include "../math/box2d.h"
#include "../math/vector3.h"

struct ScreenClipper {
    float pointTransform[4][4];
};

struct Camera;

void screenClipperInit(struct ScreenClipper* clipper, float transform[4][4]);
void screenClipperInitWithCamera(struct ScreenClipper* clipper, struct Camera* camera, float aspectRatio, float modelTransform[4][4]);

void screenClipperBoundingPoints(struct ScreenClipper* clipper, struct Vector3* input, unsigned pointCount, struct Box2D* output);

#endif
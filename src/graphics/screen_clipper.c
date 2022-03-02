
#include "screen_clipper.h"
#include "../scene/camera.h"
#include "../math/vector4.h"
#include "../math/matrix.h"

#include <math.h>

#include <ultra64.h>

void screenClipperInit(struct ScreenClipper* clipper, float transform[4][4]) {
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = 0; j < 4; ++j) {
            clipper->pointTransform[i][j] = transform[i][j];
        }
    }
}

void screenClipperInitWithCamera(struct ScreenClipper* clipper, struct Camera* camera, float aspectRatio, float modelTransform[4][4]) {
    float projection[4][4];
    float view[4][4];

    cameraBuildProjectionMatrix(camera, projection, NULL, aspectRatio);
    cameraBuildViewMatrix(camera, view);

    float viewProjection[4][4];

    guMtxCatF(view, projection, viewProjection);
    guMtxCatF(modelTransform, viewProjection, clipper->pointTransform);
}

enum ClipperBounds {
    ClipperBoundsRight = (1 << 0),
    ClipperBoundsLeft = (1 << 1),
    ClipperBoundsUp = (1 << 2),
    ClipperBoundsDown = (1 << 3),
    ClipperBoundsFront = (1 << 4),
    ClipperBoundsW = (1 << 5),
};

int screenClipperGetOutOfBounds(struct Vector4* transformed) {
    int result = 0;

    if (transformed->x > transformed->w) {
        result |= ClipperBoundsRight;
    }

    if (transformed->x < -transformed->w) {
        result |= ClipperBoundsLeft;
    }

    if (transformed->y > transformed->w) {
        result |= ClipperBoundsUp;
    }

    if (transformed->y < -transformed->w) {
        result |= ClipperBoundsDown;
    }

    if (transformed->z < -transformed->w) {
        result |= ClipperBoundsFront;
    }

    if (transformed->w <= 0.0001f) {
        result |= ClipperBoundsW;
    }

    return result;
}

float determineClippingDistance(float x0, float x1, float w0, float w1) {
    float denominator = (x1 - x0) - (w1 - w0);

    if (fabsf(denominator) < 0.00001f) {
        return 1.0f;
    }

    return (w0 - x0) / denominator;
}

void screenClipperClip(struct Vector4* from, struct Vector4* to, int clippingMask, struct Vector4* output) {
    float t = 1.0f;
    float check;

    if ((clippingMask & ClipperBoundsRight) && (check = determineClippingDistance(from->x, to->x, from->w, to->w)) < t) {
        t = check;
    }

    if ((clippingMask & ClipperBoundsLeft) && (check = determineClippingDistance(from->x, to->x, -from->w, -to->w)) < t) {
        t = check;
    }

    if ((clippingMask & ClipperBoundsUp) && (check = determineClippingDistance(from->y, to->y, from->w, to->w)) < t) {
        t = check;
    }

    if ((clippingMask & ClipperBoundsDown) && (check = determineClippingDistance(from->y, to->y, -from->w, -to->w)) < t) {
        t = check;
    }

    if ((clippingMask & ClipperBoundsFront) && (check = determineClippingDistance(from->z, to->z, -from->w, -to->w)) < t) {
        t = check;
    }

    vector4Lerp(from, to, t, output);
}

void screenClipperIncludePoint(struct Vector4* point, struct Box2D* output) {
    struct Vector2 viewportPos;
    float invW = 1.0f / point->w;
    viewportPos.x = point->x * invW;
    viewportPos.y = point->y * invW;

    vector2Min(&output->min, &viewportPos, &output->min);
    vector2Max(&output->max, &viewportPos, &output->max);
}

void screenClipperBoundingPoints(struct ScreenClipper* clipper, struct Vector3* input, unsigned pointCount, struct Box2D* output) {
    vector2Scale(&gOneVec2, -1.0f, &output->max);
    output->min = gOneVec2;

    if (pointCount == 0) {
        return;
    }

    struct Vector4 previousPoint;
    matrixVec3Mul(clipper->pointTransform, &input[pointCount - 1], &previousPoint);
    int prevIsClipped = screenClipperGetOutOfBounds(&previousPoint);

    for (unsigned i = 0; i < pointCount; ++i) {
        struct Vector4 transformedPoint;

        matrixVec3Mul(clipper->pointTransform, &input[i], &transformedPoint);

        int isClipped = screenClipperGetOutOfBounds(&transformedPoint);

        int clippedBoundary = prevIsClipped | isClipped;

        if (clippedBoundary && (!isClipped || !prevIsClipped)) {
            struct Vector4 clipped;
            if (isClipped) {
                screenClipperClip(&previousPoint, &transformedPoint, clippedBoundary, &clipped);
            } else {
                screenClipperClip(&transformedPoint, &previousPoint, clippedBoundary, &clipped);
            }

            screenClipperIncludePoint(&clipped, output);
        }

        if (!isClipped) {
            screenClipperIncludePoint(&transformedPoint, output);
        } else {
            if (!(isClipped & ClipperBoundsW)) {
                if (isClipped & ClipperBoundsLeft) {
                    output->min.x = -1.0f;
                } else if (isClipped & ClipperBoundsRight) {
                    output->max.x = 1.0f;
                } 

                if (isClipped & ClipperBoundsDown) {
                    output->min.y = -1.0f;
                } else if (isClipped & ClipperBoundsUp) {
                    output->max.y = 1.0f;
                } 
            }
        }

        previousPoint = transformedPoint;
        prevIsClipped = isClipped;
    }

    struct Vector2 negativeOne;
    vector2Scale(&gOneVec2, -1.0f, &negativeOne);
    vector2Max(&output->min, &negativeOne, &output->min);
    vector2Min(&output->max, &gOneVec2, &output->max);
}
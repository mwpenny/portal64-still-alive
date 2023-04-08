
#include "screen_clipper.h"
#include "../scene/camera.h"
#include "../math/vector4.h"
#include "../math/matrix.h"
#include "../math/mathf.h"
#include "./graphics.h"

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

float determineClippingDistance(float x0, float x1, float w0, float w1) {
    float denominator = (x1 - x0) - (w1 - w0);

    if (fabsf(denominator) < 0.00001f) {
        return 1.0f;
    }

    return (w0 - x0) / denominator;
}

void screenClipperIncludePoint(struct Vector4* point, struct Box2D* output) {
    struct Vector2 viewportPos;
    float invW = 1.0f / point->w;
    viewportPos.x = point->x * invW;
    viewportPos.y = point->y * invW;

    vector2Min(&output->min, &viewportPos, &output->min);
    vector2Max(&output->max, &viewportPos, &output->max);
}

unsigned screenClipperClipBoundary(struct ScreenClipper* clipper, struct Vector4* input, struct Vector4* output, unsigned pointCount, int axis, int direction, int oppositeSide) {
    if (pointCount == 0) {
        return 0;
    }

    unsigned outputPointCount = 0;

    struct Vector4* previous = &input[pointCount - 1];
    float pos = VECTOR3_AS_ARRAY(previous)[axis];
    float wReference = previous->w;
    float posReference = direction < 0 ? -pos : pos;
    int wasInside = oppositeSide ? (posReference > wReference) : (posReference < wReference);

    for (unsigned i = 0; i < pointCount; ++i) {
        struct Vector4* current = &input[i];

        pos = VECTOR3_AS_ARRAY(current)[axis];
        wReference = current->w;
        posReference = direction < 0 ? -pos : pos;

        int isInside = oppositeSide ? (posReference > wReference) : (posReference < wReference);

        if (isInside != wasInside) {
            float lerp = determineClippingDistance(VECTOR3_AS_ARRAY(previous)[axis], pos, direction < 0 ? -previous->w : previous->w, direction < 0 ? -wReference : wReference);
            vector4Lerp(previous, current, lerp, &output[outputPointCount]);
            ++outputPointCount;
        }

        if (isInside) {
            output[outputPointCount] = input[i];
            ++outputPointCount;
        }

        previous = current;
        wasInside = isInside;
    }

    return outputPointCount;
}

// 12 is actually 3 pixels since it is a fixed point number with 2 decimal points
#define PIXEL_EXPAND_COUNT  12

void screenClipperBoundingPoints(struct ScreenClipper* clipper, struct Vector3* input, unsigned pointCount, struct Box2D* output) {
    vector2Scale(&gOneVec2, -1.0f, &output->max);
    output->min = gOneVec2;

    if (pointCount == 0) {
        return;
    }

    struct Vector4 clipBuffer[16];
    struct Vector4 clipBufferSwap[16];

    for (unsigned i = 0; i < pointCount; ++i) {
        matrixVec3Mul(clipper->pointTransform, &input[i], &clipBuffer[i]);
    }

    pointCount = screenClipperClipBoundary(clipper, clipBuffer, clipBufferSwap, pointCount, 0, 1, 0);
    pointCount = screenClipperClipBoundary(clipper, clipBufferSwap, clipBuffer, pointCount, 0, -1, 0);
    pointCount = screenClipperClipBoundary(clipper, clipBuffer, clipBufferSwap, pointCount, 1, 1, 0);
    pointCount = screenClipperClipBoundary(clipper, clipBufferSwap, clipBuffer, pointCount, 1, -1, 0);

    clipper->nearPolygonCount = screenClipperClipBoundary(clipper, clipBuffer, clipBufferSwap, pointCount, 2, -1, 1);

    for (int i = 0; i < clipper->nearPolygonCount; ++i) {
        struct Vector4* point = &clipBufferSwap[i];
        float invW = 1.0f / point->w;
        clipper->nearPolygon[i].x = (short)(point->x * invW * (SCREEN_WD << 1) + (SCREEN_WD << 1));
        clipper->nearPolygon[i].y = (short)(point->y * invW * (SCREEN_HT << 1) + (SCREEN_HT << 1));
    }

    // expand the polygon a few pixels
    for (int i = 0; i < clipper->nearPolygonCount; ++i) {
        struct Vector2s16* curr = &clipper->nearPolygon[i];
        struct Vector2s16* next = &clipper->nearPolygon[(i + 1) % clipper->nearPolygonCount];

        struct Vector2s16 offset;
        vector2s16Sub(next, curr, &offset);

        if (abs(offset.x) > abs(offset.y)) {
            if (curr->x < next->x) {
                curr->x -= PIXEL_EXPAND_COUNT;
                next->x += PIXEL_EXPAND_COUNT;
            } else {
                curr->x += PIXEL_EXPAND_COUNT;
                next->x -= PIXEL_EXPAND_COUNT;
            }
        } else {
            if (curr->y < next->y) {
                curr->y -= PIXEL_EXPAND_COUNT;
                next->y += PIXEL_EXPAND_COUNT;
            } else {
                curr->y += PIXEL_EXPAND_COUNT;
                curr->y -= PIXEL_EXPAND_COUNT;
            }
        }
    }

    for (unsigned i = 0; i < pointCount; ++i) {
        screenClipperIncludePoint(&clipBuffer[i], output);
    }

    struct Vector2 negativeOne;
    vector2Scale(&gOneVec2, -1.0f, &negativeOne);
    vector2Max(&output->min, &negativeOne, &output->min);
    vector2Min(&output->max, &gOneVec2, &output->max);
}
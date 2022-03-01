
#include "screen_clipper.h"
#include "../scene/camera.h"
#include "../math/vector4.h"
#include "../math/matrix.h"

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

void screenClipperBoundingPoints(struct ScreenClipper* clipper, struct Vector3* input, unsigned pointCount, struct Box2D* output) {
    vector2Scale(&gOneVec2, -1.0f, &output->max);
    output->min = gOneVec2;

    for (unsigned i = 0; i < pointCount; ++i) {
        struct Vector4 transformedPoint;

        matrixVec3Mul(clipper->pointTransform, &input[i], &transformedPoint);

        int isClipped = screenClipperGetOutOfBounds(&transformedPoint);

        if (!isClipped) {
            struct Vector2 viewportPos;
            viewportPos.x = transformedPoint.x / transformedPoint.w;
            viewportPos.y = transformedPoint.y / transformedPoint.w;

            vector2Min(&output->min, &viewportPos, &output->min);
            vector2Max(&output->max, &viewportPos, &output->max);
        } else {
            // TODO find edge intersections
        }
    }
}
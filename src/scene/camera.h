#ifndef _CAMERA_H
#define _CAMERA_H

#include <ultra64.h>

#include "math/quaternion.h"
#include "math/vector3.h"
#include "math/transform.h"
#include "math/plane.h"
#include "graphics/renderstate.h"

#define CLIPPING_PLANE_COUNT    5

struct FrustrumCullingInformation {
    struct Plane clippingPlanes[CLIPPING_PLANE_COUNT];
};

struct Camera {
    struct Transform transform;
    float nearPlane;
    float farPlane;
    float fov;
};

void cameraInit(struct Camera* camera, float fov, float near, float far);
void cameraBuildViewMatrix(struct Camera* camera, float matrix[4][4]);
void cameraBuildProjectionMatrix(struct Camera* camera, float matrix[4][4], u16* perspectiveNorm, float aspectRatio);
Mtx* cameraSetupMatrices(struct Camera* camera, struct RenderState* renderState, float aspectRatio, u16* perspNorm, Vp* viewport, struct FrustrumCullingInformation* clippingInfo);

#endif
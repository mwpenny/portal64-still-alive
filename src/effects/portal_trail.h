#ifndef __PORTAL_TRAIL_H__
#define __PORTAL_TRAIL_H__

#include <ultra64.h>

#include "graphics/renderstate.h"
#include "levels/material_state.h"
#include "scene/camera.h"
#include "math/transform.h"
#include "math/vector3.h"

#define PORTAL_TRAIL_SPEED 50.0f

struct PortalTrail {
    Mtx segmentOffset;
    Mtx baseTransform[2];

    struct Transform transform;
    struct Vector3 direction;
    short currentBaseTransform;
    float headDistance;
    float targetDistance;
};

void portalTrailInit(struct PortalTrail* trail);
void portalTrailPlay(struct PortalTrail* trail, struct Vector3* from, struct Vector3* to);
void portalTrailUpdate(struct PortalTrail* trail);
void portalTrailRender(
    struct PortalTrail* trail,
    struct RenderState* renderState,
    struct MaterialState* materialState,
    struct Camera* fromCamera,
    int portalIndex
);

#endif

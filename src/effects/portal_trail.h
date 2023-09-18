#ifndef __PORTAL_TRAIL_H__
#define __PORTAL_TRAIL_H__

#include <ultra64.h>

#include "../math/vector3.h"
#include "../math/ray.h"
#include "../math/transform.h"
#include "../graphics/renderstate.h"
#include "../levels/material_state.h"

struct PortalTrail {
    struct Transform trailTransform;
    Mtx sectionOffset;
    Mtx baseTransform[2];
    struct Vector3 chunkOffset;
    short currentBaseTransform;
};

void portalTrailInit(struct PortalTrail* trail);
void portalTrailPlay(struct PortalTrail* trail, struct Vector3* from, struct Vector3* to);
void portalTrailUpdate(struct PortalTrail* trail, float distance);
void portalTrailRender(struct PortalTrail* trail, struct RenderState* renderState, struct MaterialState* materialState);

#endif
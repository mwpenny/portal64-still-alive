#ifndef __PORTAL_H__
#define __PORTAL_H__

#include "../math/transform.h"
#include "../math/plane.h"
#include "../math/vector2s16.h"
#include "../math/box3d.h"
#include "../graphics/renderstate.h"
#include "camera.h"
#include "static_scene.h"
#include "./portal_surface.h"
#include "../physics/collision_object.h"

#define PORTAL_RENDER_DEPTH_MAX  8
#define PORTAL_LOOP_SIZE    8

enum PortalFlags {
    PortalFlagsOddParity = (1 << 0),
    PortalFlagsPlayerPortal = (1 << 2),
};

struct Portal {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    short dynamicId;
    enum PortalFlags flags;
    float opacity;
    float scale;
    struct Vector2s16 originCentertedLoop[PORTAL_LOOP_SIZE];
    struct Vector2s16 fullSizeLoopCenter;
    short portalSurfaceIndex;
    short colliderIndex;
    // used to attach portals to moving surfaces
    short transformIndex;
    struct Vector3 relativePos;
    struct Quaternion relativeRotation;
};

#define NO_PORTAL 0xFF

#define PORTAL_RENDER_TYPE_VISIBLE_0    (1 << 0)
#define PORTAL_RENDER_TYPE_VISIBLE_1    (1 << 1)
#define PORTAL_RENDER_TYPE_ENABLED_0    (1 << 2)
#define PORTAL_RENDER_TYPE_ENABLED_1    (1 << 3)

#define PORTAL_RENDER_TYPE_SECOND_CLOSER    (1 << 4)

#define PORTAL_RENDER_TYPE_VISIBLE(portalIndex) (PORTAL_RENDER_TYPE_VISIBLE_0 << (portalIndex))
#define PORTAL_RENDER_TYPE_ENABLED(portalIndex) (PORTAL_RENDER_TYPE_ENABLED_0 << (portalIndex))

extern struct Vector3 gPortalOutline[PORTAL_LOOP_SIZE];

void portalInit(struct Portal* portal, enum PortalFlags flags);
void portalUpdate(struct Portal* portal, int isOpen);

void portalCalculateBB(struct Transform* portalTransform, struct Box3D* bb);

int portalAttachToSurface(struct Portal* portal, struct PortalSurface* surface, int surfaceIndex, struct Transform* portalAt, int just_checking);
void portalCheckForHoles(struct Portal* portals);

// data should be of type struct Transform
int minkowsiSumAgainstPortal(void* data, struct Vector3* direction, struct Vector3* output);

#endif
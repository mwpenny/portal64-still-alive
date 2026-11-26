#ifndef __PORTAL_GUN_H__
#define __PORTAL_GUN_H__

#include "effects/portal_trail.h"
#include "graphics/renderstate.h"
#include "math/transform.h"
#include "physics/collision_object.h"
#include "physics/rigid_body.h"
#include "player/player.h"
#include "scene/camera.h"
#include "scene/dynamic_scene.h"
#include "sk64/skeletool_animator.h"
#include "sk64/skeletool_armature.h"

#define DEFAULT_PORTAL_GUN_FOV 42.45f

struct PortalGunProjectile {
    struct PortalTrail trail;
    struct Ray positionDirection;
    struct Vector3 upDir;  // Used to orient floor and ceiling portals
    struct Vector3 effectOffset;

    float distance;
    float maxDistance;
    short roomIndex;
};

struct PortalGun {
    struct PortalGunProjectile projectiles[2];
    struct SKArmature armature;
    struct SKAnimator animator;
    struct Quaternion rotation;
    int portalGunVisible;
    float shootTimer;
    float fov;
};

void portalGunInit(struct PortalGun* portalGun, struct Player* player, float fov, int isFreshStart);
void portalGunUpdate(struct PortalGun* portalGun, struct Player* player);
void portalGunOnDeserialize(struct PortalGun* portalGun, struct Player* player);
void portalGunRender(
    struct PortalGun* portalGun,
    struct RenderState* renderState,
    struct Camera* fromCamera,
    int lastFiredIndex
);

void portalGunFire(
    struct PortalGun* portalGun,
    int portalIndex,
    struct Ray* ray,
    struct Transform* lookTransform,
    struct Vector3* upDir,
    int roomIndex
);
void portalGunFireWorld(
    struct PortalGun* portalGun,
    int portalIndex,
    struct Vector3* from,
    struct Vector3* to,
    int roomIndex
);
int portalGunIsFiring(struct PortalGun* portalGun);

void portalGunPlayDraw(struct PortalGun* portalGun);
void portalGunPlayFizzle(struct PortalGun* portalGun);
void portalGunPlayPickup(struct PortalGun* portalGun);
void portalGunPlayRelease(struct PortalGun* portalGun);

#endif

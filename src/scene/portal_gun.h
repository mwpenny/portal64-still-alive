#ifndef __PORTAL_GUN_H__
#define __PORTAL_GUN_H__

#include "../physics/collision_object.h"
#include "../math/transform.h"
#include "../graphics/renderstate.h"
#include "../physics/rigid_body.h"
#include "../physics/collision_object.h"
#include "../scene/dynamic_scene.h"
#include "../player/player.h"
#include "../util/time.h"
#include "../effects/portal_trail.h"
#include "../scene/camera.h"

struct PortalGunProjectile {
    struct Ray positionDirection;
    // used to orient portals that land on the floor or ceiling
    struct Vector3 playerUp;

    struct Vector3 effectOffset;

    struct PortalTrail trail;

    float distance;
    float maxDistance;

    short roomIndex;
};

struct PortalGun {
    struct RigidBody rigidBody;
    int portalGunVisible;
    float shootAnimationTimer;
    float shootTotalAnimationTimer;

    struct PortalGunProjectile projectiles[2];
};

void portalGunInit(struct PortalGun* portalGun, struct Transform* at);
// void portalGunDummyRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);
void portalGunUpdate(struct PortalGun* portalGun, struct Player* player);
void portalGunRenderReal(struct PortalGun* portalGun, struct RenderState* renderState, struct Camera* fromCamera, int portalGunVisible);

void portalGunFire(struct PortalGun* portalGun, int portalIndex, struct Ray* ray, struct Vector3* playerUp, int roomIndex);
void portalGunFireWorld(struct PortalGun* portalGun, int portalIndex, struct Vector3* from, struct Vector3* to, int roomIndex);
int portalGunIsFiring(struct PortalGun* portalGun);

#endif
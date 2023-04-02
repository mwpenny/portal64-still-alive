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

struct PortalGun {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    int portalGunVisible;
    float shootAnimationTimer;
};

void portalGunInit(struct PortalGun* portalGun, struct Transform* at);
// void portalGunDummyRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);
void portalGunUpdate(struct PortalGun* portalGun, struct Player* player);
void portalGunRenderReal(struct PortalGun* portalGun, struct RenderState* renderState);

#endif
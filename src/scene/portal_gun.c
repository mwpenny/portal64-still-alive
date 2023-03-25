#include "portal_gun.h"

#include "../physics/collision_scene.h"
#include "../physics/collision_cylinder.h"
#include "models/models.h"

struct Vector2 gGunColliderEdgeVectors[] = {
    {0.0f, 1.0f},
    {0.707f, 0.707f},
    {1.0f, 0.0f},
    {0.707f, -0.707f},
};

struct CollisionQuad gGunColliderFaces[8];

struct CollisionCylinder gGunCollider = {
    0.05f,
    0.1f,
    gGunColliderEdgeVectors,
    sizeof(gGunColliderEdgeVectors) / sizeof(*gGunColliderEdgeVectors),
    gGunColliderFaces,
};

struct ColliderTypeData gGunColliderData = {
    CollisionShapeTypeCylinder,
    &gGunCollider,
    0.0f,
    0.6f,
    &gCollisionCylinderCallbacks,
};

void portalGunInit(struct PortalGun* portalGun, struct Transform* at){
    collisionObjectInit(&portalGun->collisionObject, &gGunColliderData, &portalGun->rigidBody, 1.0f, 0);
    collisionSceneAddDynamicObject(&portalGun->collisionObject);
    portalGun->rigidBody.transform = *at;
    portalGun->rigidBody.transform.scale = gOneVec;
    portalGun->rigidBody.currentRoom = 0;
    portalGun->rigidBody.velocity = gZeroVec;
    portalGun->rigidBody.angularVelocity = gZeroVec;
    portalGun->dynamicId = dynamicSceneAdd(portalGun, portalGunDummyRender, &portalGun->rigidBody.transform.position, 0.05f);
    portalGun->portalGunVisible = 0;

    collisionObjectUpdateBB(&portalGun->collisionObject);
    dynamicSceneSetRoomFlags(portalGun->dynamicId, ROOM_FLAG_FROM_INDEX(portalGun->rigidBody.currentRoom));

}

void portalGunDummyRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState){
    return;
}

void portalGunRenderReal(struct PortalGun* portalGun, struct RenderState* renderState){
    if (portalGun->portalGunVisible){
        portalGun->rigidBody.transform.scale = gOneVec;
        Mtx* matrix = renderStateRequestMatrices(renderState, 1);

        if (!matrix) {
            return;
        }

        transformToMatrixL(&portalGun->rigidBody.transform, matrix, SCENE_SCALE);
        gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
        gSPDisplayList(renderState->dl++, v_portal_gun_gfx);
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
    }
}



void portalGunUpdate(struct PortalGun* portalGun, struct Player* player){
    if (player->flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun)){
        portalGun->portalGunVisible = 1;
    }
    else{
        portalGun->portalGunVisible = 0;
    }
    dynamicSceneSetRoomFlags(portalGun->dynamicId, ROOM_FLAG_FROM_INDEX(portalGun->rigidBody.currentRoom));
}
#include "turret.h"

#include "defs.h"
#include "physics/collision_scene.h"
#include "scene/dynamic_scene.h"
#include "util/dynamic_asset_loader.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"

static struct CollisionBox gTurretCollisionBox = {
    {0.2f, 0.554f, 0.4f}
};

static struct ColliderTypeData gTurretCollider = {
    CollisionShapeTypeBox,
    &gTurretCollisionBox,
    0.0f,
    1.0f,
    &gCollisionBoxCallbacks
};

static struct Vector3 gTurretOriginOffset = { 0.0f, 0.554f, 0.0f };
static struct Vector3 gTurretLaserOffset = { 0.0f, 0.0425f, 0.0f };

#define TURRET_MASS             3.0f
#define TURRET_COLLISION_LAYERS (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_TURRET_SHOTS)

static void turretRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Turret* turret = (struct Turret*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    // Render without origin offset
    struct Vector3 transformedOriginOffset;
    quatMultVector(&turret->rigidBody.transform.rotation, &gTurretOriginOffset, &transformedOriginOffset);

    struct Transform transform = turret->rigidBody.transform;
    vector3Sub(&transform.position, &transformedOriginOffset, &transform.position);
    transformToMatrixL(&transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, turret->armature.numberOfBones);

    if (!armature) {
        return;
    }

    skCalculateTransforms(&turret->armature, armature);

    dynamicRenderListAddDataTouchingPortal(
        renderList,
        turret->armature.displayList,
        matrix,
        TURRET_INDEX,
        &turret->rigidBody.transform.position,
        armature,
        turret->rigidBody.flags
    );
}

void turretInit(struct Turret* turret, struct TurretDefinition* definition) {
    // TODO:
    // * Fizzleable

    turret->definition = definition;

    collisionObjectInit(&turret->collisionObject, &gTurretCollider, &turret->rigidBody, TURRET_MASS, TURRET_COLLISION_LAYERS);
    collisionSceneAddDynamicObject(&turret->collisionObject);

    turret->rigidBody.transform.rotation = definition->rotation;
    turret->rigidBody.transform.scale = gOneVec;
    turret->rigidBody.flags |= RigidBodyFlagsGrabbable;
    turret->rigidBody.currentRoom = definition->roomIndex;

    struct Vector3 originOffset;
    quatMultVector(&turret->rigidBody.transform.rotation, &gTurretOriginOffset, &originOffset);
    vector3Add(&definition->position, &originOffset, &turret->rigidBody.transform.position);

    collisionObjectUpdateBB(&turret->collisionObject);

    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_TURRET_01_DYNAMIC_ANIMATED_MODEL);
    skArmatureInit(&turret->armature, armature->armature);

    laserInit(&turret->laser, &turret->rigidBody.transform, &gTurretLaserOffset);

    turret->dynamicId = dynamicSceneAdd(turret, turretRender, &turret->rigidBody.transform.position, 0.75f);
    dynamicSceneSetRoomFlags(turret->dynamicId, ROOM_FLAG_FROM_INDEX(turret->rigidBody.currentRoom));
}

void turretUpdate(struct Turret* turret) {
    if (turret->collisionObject.flags & COLLISION_OBJECT_PLAYER_STANDING) {
        turret->collisionObject.flags &= ~COLLISION_OBJECT_PLAYER_STANDING;
    }

    dynamicSceneSetRoomFlags(turret->dynamicId, ROOM_FLAG_FROM_INDEX(turret->rigidBody.currentRoom));
}

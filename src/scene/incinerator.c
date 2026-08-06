#include "incinerator.h"

#include "effects/effect_definitions.h"
#include "physics/collision_scene.h"
#include "scene.h"
#include "scene/dynamic_scene.h"
#include "util/dynamic_asset_loader.h"
#include "util/frame_time.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/props_bts/glados_aperturedoor.h"
#include "codegen/assets/models/props_bts/glados_aperturedoor_collision.h"

#define INCINERATOR_SCALE                   1024.0f  // Avoids z-fighting
#define INCINERATOR_COLLISION_LAYERS        (COLLISION_LAYERS_STATIC | COLLISION_LAYERS_TANGIBLE)

#define INCINERATOR_SMOKE_OFFSET            -0.5f
#define INCINERATOR_SMOKE_OPEN_DELAY        0.25f
#define INCINERATOR_SMOKE_RUSH_MIN_DELAY    0.0625f
#define INCINERATOR_SMOKE_RUSH_MAX_DELAY    0.125f
#define INCINERATOR_SMOKE_MIN_DELAY         0.25f
#define INCINERATOR_SMOKE_MAX_DELAY         0.75f

static struct ColliderTypeData sIncineratorColliderType = {
    CollisionShapeTypeMesh,
    &props_bts_glados_aperturedoor_collision_collider,
    0.0f, 0.6f,
    &gMeshColliderCallbacks
};

static void incineratorRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Incinerator* incinerator = (struct Incinerator*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);
    if (!matrix) {
        return;
    }

    struct Transform transform = incinerator->rigidBody.transform;
    vector3Scale(&transform.scale, &transform.scale, SCENE_SCALE / INCINERATOR_SCALE);
    transformToMatrixL(&transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, incinerator->armature.numberOfBones);
    if (!armature) {
        return;
    }

    skCalculateTransforms(&incinerator->armature, armature);

    dynamicRenderListAddData(
        renderList,
        incinerator->armature.displayList,
        matrix,
        INCINERATOR_INDEX,
        &incinerator->rigidBody.transform.position,
        armature
    );
}

static void incineratorUpdateCollision(struct Incinerator* incinerator) {
    // This is global - the last incinerator wins
    short* faceCollisionLayers = &props_bts_glados_aperturedoor_collision_collider
        .children[PROPS_BTS_GLADOS_APERTUREDOOR_COLLISION_BLADES_COLLISION_INDEX]
        .collisionLayers;

    *faceCollisionLayers = incinerator->isOpen ? 0 : INCINERATOR_COLLISION_LAYERS;
}

void incineratorInit(struct Incinerator* incinerator, struct IncineratorDefinition* definition) {
    collisionObjectInit(
        &incinerator->collisionObject,
        &sIncineratorColliderType,
        &incinerator->rigidBody,
        1.0f,
        INCINERATOR_COLLISION_LAYERS
    );
    rigidBodyMarkKinematic(&incinerator->rigidBody);
    collisionSceneAddDynamicObject(&incinerator->collisionObject);

    incinerator->rigidBody.transform.position = definition->position;
    incinerator->rigidBody.transform.rotation = definition->rotation;
    incinerator->rigidBody.transform.scale = gOneVec;

    collisionObjectUpdateBB(&incinerator->collisionObject);

    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_BTS_GLADOS_APERTUREDOOR_DYNAMIC_ANIMATED_MODEL);
    skArmatureInit(&incinerator->armature, armature->armature);
    skAnimatorInit(&incinerator->animator, armature->armature->numberOfBones);

    incinerator->dynamicId = dynamicSceneAdd(
        incinerator,
        incineratorRender,
        &incinerator->rigidBody.transform.position,
        2.0f
    );
    dynamicSceneSetRoomFlags(incinerator->dynamicId, ROOM_FLAG_FROM_INDEX(definition->roomIndex));

    incinerator->smokeTimer = 0.0f;
    incinerator->signalIndex = definition->signalIndex;
    incinerator->isOpen = 0;
    incineratorUpdateCollision(incinerator);
}

static void incineratorUpdateSmoke(struct Incinerator* incinerator) {
    if (incinerator->smokeTimer > 0.0f) {
        incinerator->smokeTimer -= FIXED_DELTA_TIME;
        return;
    }

    struct ParticleEffectDefinition* effectDef;
    if (skAnimatorIsRunning(&incinerator->animator)) {
        // Blades are opening
        effectDef = &gSmokeFast;
        incinerator->smokeTimer = randomInRangef(
            INCINERATOR_SMOKE_RUSH_MIN_DELAY,
            INCINERATOR_SMOKE_RUSH_MAX_DELAY
        );
    } else {
        effectDef = &gSmoke;
        incinerator->smokeTimer = randomInRangef(
            INCINERATOR_SMOKE_MIN_DELAY,
            INCINERATOR_SMOKE_MAX_DELAY
        );
    }

    struct Vector3 origin;
    vector3AddScaled(
        &incinerator->rigidBody.transform.position,
        &incinerator->rigidBody.rotationBasis.y,
        INCINERATOR_SMOKE_OFFSET,
        &origin
    );

    effectsParticlePlay(
        &gScene.effects,
        effectDef,
        &origin,
        &gUp,
        NULL
    );
}

void incineratorUpdate(struct Incinerator* incinerator) {
    int animationDirection = incinerator->isOpen ? 1 : -1;
    skAnimatorUpdate(
        &incinerator->animator,
        incinerator->armature.pose,
        FIXED_DELTA_TIME * animationDirection
    );

    if (signalsRead(incinerator->signalIndex) != incinerator->isOpen) {
        struct SKAnimationClip* clip = dynamicAssetClip(
            PROPS_BTS_GLADOS_APERTUREDOOR_DYNAMIC_ANIMATED_MODEL,
            PROPS_BTS_GLADOS_APERTUREDOOR_ARMATURE_OPEN_CLIP_INDEX
        );

        float startTime = SK_ANIMATION_CLIP_START(clip, incinerator->isOpen);
        skAnimatorEnsureClipRunning(&incinerator->animator, clip, startTime, 0);

        incinerator->isOpen ^= 1;
        incinerator->smokeTimer = incinerator->isOpen ? INCINERATOR_SMOKE_OPEN_DELAY : 0.0f;
        incineratorUpdateCollision(incinerator);
    }

    if (incinerator->isOpen) {
        incineratorUpdateSmoke(incinerator);
    }
}

void incineratorOnDeserialize(struct Incinerator* incinerator) {
    if (signalsRead(incinerator->signalIndex)) {
        struct SKAnimationClip* clip = dynamicAssetClip(
            PROPS_BTS_GLADOS_APERTUREDOOR_DYNAMIC_ANIMATED_MODEL,
            PROPS_BTS_GLADOS_APERTUREDOOR_ARMATURE_OPEN_CLIP_INDEX
        );

        skAnimatorRunClip(&incinerator->animator, clip, SK_ANIMATION_CLIP_DURATION(clip), 0);
        incinerator->smokeTimer = 0.01f;  // Don't play smoke rush

        incinerator->isOpen = 1;
        incineratorUpdateCollision(incinerator);
    }
}

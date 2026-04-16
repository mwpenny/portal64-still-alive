#include "ball.h"

#include "audio/soundplayer.h"
#include "audio/clips.h"
#include "controls/rumble_pak_clip.h"
#include "dynamic_scene.h"
#include "effects/effect_definitions.h"
#include "physics/collision_scene.h"
#include "physics/collision_box.h"
#include "scene.h"
#include "util/frame_time.h"

#include "codegen/assets/models/grav_flare.h"
#include "codegen/assets/models/fleck_ash2.h"
#include "codegen/assets/materials/static.h"

#define BALL_RADIUS 0.1f

static struct CollisionBox sBallCollisionBox = {
    {BALL_RADIUS, BALL_RADIUS, BALL_RADIUS}
};

static struct ColliderTypeData sBallCollider = {
    CollisionShapeTypeBox,
    &sBallCollisionBox,
    1.0f,
    0.0f,
    &gCollisionBoxCallbacks,
};

static struct BallBurnMark sBurnMarks[MAX_BURN_MARKS];
static short sCurrentBurnIndex;

static unsigned char sBallBounceRumbleData[] = {
    0xFA
};
static struct RumblePakWave sBallBounceRumbleWave = {
    .samples = sBallBounceRumbleData,
    .sampleCount = 4,
    .samplesPerSecond = 20,
};

void ballBurnMarkInit() {
    sCurrentBurnIndex = 0;

    for (int i = 0; i < 3; ++i) {
        sBurnMarks[i].dynamicId = INVALID_DYNAMIC_OBJECT;
    }
}

void ballBurnFilterOnPortal(struct Transform* portalTransform, int portalIndex) {
    for (int i = 0; i < MAX_BURN_MARKS; ++i) {
        struct BallBurnMark* burnMark = &sBurnMarks[i];
        
        if (burnMark->dynamicId == -1) {
            continue;
        }

        if (collisionSceneIsTouchingSinglePortal(&burnMark->at, &burnMark->normal, portalTransform, portalIndex)) {
            dynamicSceneRemove(burnMark->dynamicId);
            burnMark->dynamicId = INVALID_DYNAMIC_OBJECT;
        }
    }
}

void ballRender(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct Ball* ball = (struct Ball*)data;
    struct Transform transform;
    transform.position = ball->rigidBody.transform.position;
    transform.rotation = fromView->rotation;
    vector3Scale(&gOneVec, &transform.scale, BALL_RADIUS);

    Mtx* mtx = renderStateRequestMatrices(renderScene->renderState, 1);

    transformToMatrixL(&transform, mtx, SCENE_SCALE);

    renderSceneAdd(renderScene, grav_flare_model_gfx, mtx, GRAV_FLARE_INDEX, &ball->rigidBody.transform.position, NULL);
}

void ballBurnRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct BallBurnMark* burn = (struct BallBurnMark*)data;

    dynamicRenderListAddData(
        renderList,
        fleck_ash2_model_gfx,
        &burn->matrix,
        FLECK_ASH2_INDEX,
        &burn->at,
        NULL
    );
}

void ballInitInactive(struct Ball* ball) {
    collisionObjectInit(&ball->collisionObject, &sBallCollider, &ball->rigidBody, 1.0f, 0);

    ball->targetSpeed = 0.0f;
    ball->flags = 0;
    ball->soundLoopId = SOUND_ID_NONE;
    ball->lifetime = 0.0f;
}

void ballInit(struct Ball* ball, struct Vector3* position, struct Vector3* velocity, short startingRoom, float ballLifetime) {
    collisionObjectInit(&ball->collisionObject, &sBallCollider, &ball->rigidBody, 1.0f, 0);

    collisionSceneAddDynamicObject(&ball->collisionObject);

    ball->rigidBody.flags |= RigidBodyDisableGravity;

    ball->rigidBody.velocity = *velocity;
    ball->rigidBody.transform.position = *position;
    quatIdent(&ball->rigidBody.transform.rotation);
    ball->rigidBody.transform.scale = gOneVec;
    ball->rigidBody.currentRoom = startingRoom;
    ball->flags = 0;
    ball->lifetime = ballLifetime;
    ball->originalLifetime = ballLifetime;

    ball->targetSpeed = sqrtf(vector3MagSqrd(&ball->rigidBody.velocity));

    ball->dynamicId = dynamicSceneAddViewDependent(ball, ballRender, &ball->rigidBody.transform.position, BALL_RADIUS);

    dynamicSceneSetRoomFlags(ball->dynamicId, ROOM_FLAG_FROM_INDEX(startingRoom));

    ball->soundLoopId = soundPlayerPlay(soundsBallLoop, 1.3f, 1.0f, &ball->rigidBody.transform.position, &ball->rigidBody.velocity, SoundTypeAll);
}

void ballTurnOnCollision(struct Ball* ball) {
    ball->collisionObject.collisionLayers |= COLLISION_LAYERS_BLOCK_BALL;
}

void ballCheckBounced(struct Ball* ball) {
    struct ContactManifold* manifold = contactSolverNextManifold(&gContactSolver, &ball->collisionObject, NULL);
    if (!manifold || manifold->contactCount == 0) {
        ball->flags &= ~BallJustBounced;
        return;
    }

    if (ball->flags & BallJustBounced) {
        return;
    }

    struct CollisionObject* other;
    struct Vector3 normal;
    if (manifold->shapeA == &ball->collisionObject) {
        other = manifold->shapeB;
        vector3Negate(&manifold->normal, &normal);
    } else {
        other = manifold->shapeA;
        normal = manifold->normal;
    }

    effectsSplashPlay(&gScene.effects, &gBallBounce, &ball->rigidBody.transform.position, &normal, NULL);
    soundPlayerPlay(soundsBallBounce, 1.5f, 1.0f, &ball->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
    hudShowSubtitle(&gScene.hud, ENERGYBALL_IMPACT, SubtitleTypeCaption);
    ball->flags |= BallJustBounced;

    if (other->body != NULL) {
        if (playerIsGrabbingObject(&gScene.player, other)) {
            rumblePakClipPlay(&sBallBounceRumbleWave);
        }

        // Only add burn marks to static objects
        return;
    }

    struct BallBurnMark* burn = &sBurnMarks[sCurrentBurnIndex];
    struct BallBurnMark* lastBurn = &sBurnMarks[(sCurrentBurnIndex == 0 ? MAX_BURN_MARKS : sCurrentBurnIndex) - 1];

    struct Transform burnTransform;
    burnTransform.position = manifold->contacts[0].contactAWorld;

    // Don't double up burns
    if (lastBurn->dynamicId != INVALID_DYNAMIC_OBJECT && vector3DistSqrd(&burnTransform.position, &lastBurn->at) < 0.1f) {
        return;
    }

    ++sCurrentBurnIndex;
    if (sCurrentBurnIndex == MAX_BURN_MARKS) {
        sCurrentBurnIndex = 0;
    }

    if (burn->dynamicId == INVALID_DYNAMIC_OBJECT) {
        burn->dynamicId = dynamicSceneAdd(burn, ballBurnRender, &burn->at, 0.2f);
    }

    if (fabsf(normal.y) > 0.714f) {
        quatLook(&normal, &gRight, &burnTransform.rotation);
    } else {
        quatLook(&normal, &gUp, &burnTransform.rotation);
    }
    burnTransform.scale = gOneVec;

    transformToMatrixL(&burnTransform, &burn->matrix, SCENE_SCALE);

    burn->at = burnTransform.position;
    burn->normal = normal;
}

void ballUpdate(struct Ball* ball) {
    if (ball->targetSpeed == 0.0f || ballIsCaught(ball)) {
        return;
    }

    if (ball->rigidBody.flags & (RigidBodyFlagsCrossedPortal0 | RigidBodyFlagsCrossedPortal1)){
        ball->lifetime = ball->originalLifetime;
    }

    float currentSpeed = sqrtf(vector3MagSqrd(&ball->rigidBody.velocity));

    if (currentSpeed == 0.0f) {
        vector3Scale(&gRight, &ball->rigidBody.velocity, ball->targetSpeed);
    } else {
        vector3Scale(&ball->rigidBody.velocity, &ball->rigidBody.velocity, ball->targetSpeed / currentSpeed);
    }

    soundPlayerSetPosition(ball->soundLoopId, &ball->rigidBody.transform.position, &ball->rigidBody.velocity);

    ball->rigidBody.angularVelocity = gOneVec;
    
    dynamicSceneSetRoomFlags(ball->dynamicId, ROOM_FLAG_FROM_INDEX(ball->rigidBody.currentRoom));

    if (ball->lifetime > 0.0f) {
        ball->lifetime -= FIXED_DELTA_TIME;

        if (ball->lifetime <= 0.0f) {
            ball->targetSpeed = 0.0f;
            collisionSceneRemoveDynamicObject(&ball->collisionObject);
            dynamicSceneRemove(ball->dynamicId);
            soundPlayerStop(ball->soundLoopId);
            soundPlayerPlay(soundsBallExplode, 2.0f, 1.0f, &ball->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
            hudShowSubtitle(&gScene.hud, ENERGYBALL_EXPLOSION, SubtitleTypeCaption);
            effectsSplashPlay(&gScene.effects, &gBallBurst, &ball->rigidBody.transform.position, &gUp, NULL);
            ball->soundLoopId = SOUND_ID_NONE;
        }
    }

    ballCheckBounced(ball);
}

int ballIsActive(struct Ball* ball) {
    return ball->targetSpeed != 0.0f;
}

int ballIsCollisionOn(struct Ball* ball) {
    return ball->collisionObject.collisionLayers != 0;
}

int ballIsCaught(struct Ball* ball) {
    return (ball->flags & BallFlagsCaught) != 0;
}

void ballMarkCaught(struct Ball* ball) {
    ball->flags |= BallFlagsCaught;
    collisionSceneRemoveDynamicObject(&ball->collisionObject);
    rigidBodyMarkKinematic(&ball->rigidBody);
    soundPlayerStop(ball->soundLoopId);
    ball->soundLoopId = SOUND_ID_NONE;
}

int isColliderForBall(struct CollisionObject* collisionObject) {
    return collisionObject->collider->data == &sBallCollisionBox;
}

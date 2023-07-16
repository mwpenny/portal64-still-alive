#include "./ball.h"

#include "dynamic_scene.h"
#include "../defs.h"

#include "../physics/collision_scene.h"
#include "../physics/collision_box.h"

#include "../util/time.h"

#include "../build/assets/models/grav_flare.h"
#include "../build/assets/models/fleck_ash2.h"
#include "../build/assets/materials/static.h"

#include "../audio/soundplayer.h"
#include "../audio/clips.h"

#define BALL_RADIUS 0.1f

struct CollisionBox gBallCollisionBox = {
    {BALL_RADIUS, BALL_RADIUS, BALL_RADIUS}
};

struct ColliderTypeData gBallCollider = {
    CollisionShapeTypeBox,
    &gBallCollisionBox,
    1.0f,
    0.0f,
    &gCollisionBoxCallbacks,
};

struct BallBurnMark gBurnMarks[MAX_BURN_MARKS];
short gCurrentBurnIndex;

void ballBurnMarkInit() {
    gCurrentBurnIndex = 0;

    for (int i = 0; i < 3; ++i) {
        gBurnMarks[i].dynamicId = -1;
    }
}

void ballBurnFilterOnPortal(struct Transform* portalTransform, int portalIndex) {
    for (int i = 0; i < MAX_BURN_MARKS; ++i) {
        struct BallBurnMark* burnMark = &gBurnMarks[i];
        
        if (burnMark->dynamicId == -1) {
            continue;
        }

        if (collisionSceneIsTouchingSinglePortal(&burnMark->at, &burnMark->normal, portalTransform, portalIndex)) {
            dynamicSceneRemove(burnMark->dynamicId);
            burnMark->dynamicId = -1;
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
    collisionObjectInit(&ball->collisionObject, &gBallCollider, &ball->rigidBody, 1.0f, 0);

    ball->targetSpeed = 0.0f;
    ball->flags = 0;
    ball->soundLoopId = SOUND_ID_NONE;
    ball->lifetime = 0.0f;
}

void ballInit(struct Ball* ball, struct Vector3* position, struct Vector3* velocity, short startingRoom, float ballLifetime) {
    collisionObjectInit(&ball->collisionObject, &gBallCollider, &ball->rigidBody, 1.0f, 0);

    collisionSceneAddDynamicObject(&ball->collisionObject);

    ball->rigidBody.flags |= RigidBodyDisableGravity;

    ball->rigidBody.velocity = *velocity;
    ball->rigidBody.transform.position = *position;
    quatIdent(&ball->rigidBody.transform.rotation);
    ball->rigidBody.transform.scale = gOneVec;
    ball->rigidBody.currentRoom = startingRoom;
    ball->flags = 0;
    ball->lifetime = ballLifetime;

    ball->targetSpeed = sqrtf(vector3MagSqrd(&ball->rigidBody.velocity));

    ball->dynamicId = dynamicSceneAddViewDependant(ball, ballRender, &ball->rigidBody.transform.position, BALL_RADIUS);

    dynamicSceneSetRoomFlags(ball->dynamicId, ROOM_FLAG_FROM_INDEX(startingRoom));

    ball->soundLoopId = soundPlayerPlay(soundsBallLoop, 1.0f, 1.0f, &ball->rigidBody.transform.position, &ball->rigidBody.velocity);
}

void ballTurnOnCollision(struct Ball* ball) {
    ball->collisionObject.collisionLayers |= COLLISION_LAYERS_BLOCK_BALL;
}

void ballInitBurn(struct Ball* ball, struct ContactManifold* manifold) {
    if (!manifold || manifold->contactCount == 0) {
        return;
    }

    // only add burn marks to static objects
    if (manifold->shapeA->body != NULL) {
        return;
    }

    struct BallBurnMark* burn = &gBurnMarks[gCurrentBurnIndex];

    struct BallBurnMark* lastBurn = &gBurnMarks[(gCurrentBurnIndex == 0 ? MAX_BURN_MARKS : gCurrentBurnIndex) - 1];

    struct Transform burnTransform;
    burnTransform.position = manifold->contacts[0].contactAWorld;

    // don't double up burns
    if (vector3DistSqrd(&burnTransform.position, &lastBurn->at) < 0.1f) {
        return;
    }

    ++gCurrentBurnIndex;

    if (gCurrentBurnIndex == MAX_BURN_MARKS) {
        gCurrentBurnIndex = 0;
    }

    if (burn->dynamicId == -1) {
        burn->dynamicId = dynamicSceneAdd(burn, ballBurnRender, &burn->at, 0.2f);
    }

    quatLook(&manifold->normal, &gUp, &burnTransform.rotation);
    burnTransform.scale = gOneVec;

    transformToMatrixL(&burnTransform, &burn->matrix, SCENE_SCALE);

    soundPlayerPlay(soundsBallBounce, 1.0f, 1.0f, &burnTransform.position, &gZeroVec);

    burn->at = burnTransform.position;
    burn->normal = manifold->normal;
}

void ballUpdate(struct Ball* ball) {
    if (ball->targetSpeed == 0.0f || ballIsCaught(ball)) {
        return;
    }

    float currentSpeed = sqrtf(vector3MagSqrd(&ball->rigidBody.velocity));

    if (currentSpeed == 0.0f) {
        vector3Scale(&gRight, &ball->rigidBody.velocity, ball->targetSpeed);
    } else {
        vector3Scale(&ball->rigidBody.velocity, &ball->rigidBody.velocity, ball->targetSpeed / currentSpeed);
    }

    soundPlayerUpdatePosition(ball->soundLoopId, &ball->rigidBody.transform.position, &ball->rigidBody.velocity);

    ball->rigidBody.angularVelocity = gOneVec;
    
    dynamicSceneSetRoomFlags(ball->dynamicId, ROOM_FLAG_FROM_INDEX(ball->rigidBody.currentRoom));

    if (ball->lifetime > 0.0f) {
        ball->lifetime -= FIXED_DELTA_TIME;

        if (ball->lifetime <= 0.0f) {
            ball->targetSpeed = 0.0f;
            collisionSceneRemoveDynamicObject(&ball->collisionObject);
            dynamicSceneRemove(ball->dynamicId);
            soundPlayerStop(ball->soundLoopId);
            soundPlayerPlay(soundsBallExplode, 1.0f, 1.0f, &ball->rigidBody.transform.position, &gZeroVec);
            ball->soundLoopId = SOUND_ID_NONE;
        }
    }

    struct ContactManifold* manifold = contactSolverNextManifold(&gContactSolver, &ball->collisionObject, NULL);
    ballInitBurn(ball, manifold);
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
    return collisionObject->collider->data == &gBallCollisionBox;
}
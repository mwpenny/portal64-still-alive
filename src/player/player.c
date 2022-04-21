
#include "player.h"
#include "../controls/controller.h"
#include "../util/time.h"
#include "../defs.h"
#include "../physics/point_constraint.h"
#include "../math/mathf.h"
#include "physics/collision_sphere.h"
#include "physics/collision_scene.h"

struct Vector3 gGrabDistance = {0.0f, 0.0f, -2.5f};
struct Vector3 gCameraOffset = {0.0f, 0.0f, 0.0f};

struct CollisionSphere gPlayerCollider = {
    0.25f,
};

struct ColliderTypeData gPlayerColliderData = {
    CollisionShapeTypeSphere,
    &gPlayerCollider,
    0.1f,
    0.5f,
    &gCollisionSphereCallbacks,
};

void playerInit(struct Player* player) {
    collisionObjectInit(&player->collisionObject, &gPlayerColliderData, &player->body, 1.0f);
    player->grabbing = NULL;
    player->yaw = 0.0f;
    player->pitch = 0.0f;

    player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
}

#define PLAYER_SPEED    (5.0f)
#define PLAYER_ACCEL    (40.0f)
#define PLAYER_STOP_ACCEL    (80.0f)

#define ROTATE_RATE     (M_PI * 2.0f)
#define ROTATE_RATE_DELTA     (M_PI * 0.25f)
#define ROTATE_RATE_STOP_DELTA (M_PI * 0.25f)

void playerHandleCollision(void* data, struct ContactConstraintState* contact) {
    struct Player* player = (struct Player*)data;

    if (contact->contactCount == 1 && contact->contacts[0].penetration < 0.0f) {
        vector3AddScaled(
            &player->body.transform.position, 
            &contact->normal, 
            -contact->contacts[0].penetration, 
            &player->body.transform.position
        );
    }
}

void playerApplyPortalGrab(struct Player* player, int portalIndex) {
    if (player->grabbingThroughPortal == PLAYER_GRABBING_THROUGH_NOTHING) {
        player->grabbingThroughPortal = portalIndex;
    } else if (player->grabbingThroughPortal != portalIndex) {
        player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
    }
}

void playerUpdateGrabbedObject(struct Player* player) {
    if (player->grabbing) {
        if (player->body.flags & RigidBodyFlagsCrossedPortal0) {
            playerApplyPortalGrab(player, 1);
        }

        if (player->body.flags & RigidBodyFlagsCrossedPortal1) {
            playerApplyPortalGrab(player, 0);
        }

        if (player->grabbing->flags & RigidBodyFlagsCrossedPortal0) {
            playerApplyPortalGrab(player, 0);
        }

        if (player->grabbing->flags & RigidBodyFlagsCrossedPortal1) {
            playerApplyPortalGrab(player, 1);
        }

        struct Vector3 grabPoint;
        struct Quaternion grabRotation = player->body.transform.rotation;

        transformPoint(&player->body.transform, &gGrabDistance, &grabPoint);

        if (player->grabbingThroughPortal != PLAYER_GRABBING_THROUGH_NOTHING) {
            if (!collisionSceneIsPortalOpen()) {
                // portal was closed while holding object through it
                player->grabbing = NULL;
                return;
            }

            struct Transform pointTransform;
            collisionSceneGetPortalTransform(player->grabbingThroughPortal, &pointTransform);

            transformPoint(&pointTransform, &grabPoint, &grabPoint);
            struct Quaternion finalRotation;
            quatMultiply(&pointTransform.rotation, &grabRotation, &finalRotation);
            grabRotation = finalRotation;
        }

        pointConstraintMoveToPoint(player->grabbing, &grabPoint, 20.0f);
        pointConstraintRotateTo(player->grabbing, &grabRotation, 5.0f);
    }
}

void playerUpdate(struct Player* player, struct Transform* cameraTransform) {
    struct Vector3 forward;
    struct Vector3 right;

    struct Transform* transform = &player->body.transform;

    quatMultVector(&transform->rotation, &gForward, &forward);
    quatMultVector(&transform->rotation, &gRight, &right);

    forward.y = 0.0f;
    right.y = 0.0f;

    vector3Normalize(&gForward, &gForward);
    vector3Normalize(&gRight, &gRight);

    OSContPad* controllerInput = controllersGetControllerData(0);

    struct Vector3 targetVelocity;

    vector3Scale(&forward, &targetVelocity, -controllerInput->stick_y * PLAYER_SPEED / 80.0f);
    vector3AddScaled(&targetVelocity, &right, controllerInput->stick_x * PLAYER_SPEED / 80.0f, &targetVelocity);

    vector3MoveTowards(
        &player->body.velocity, 
        &targetVelocity, 
        vector3Dot(&player->body.velocity, &targetVelocity) > 0.0f ? PLAYER_ACCEL * FIXED_DELTA_TIME : PLAYER_STOP_ACCEL * FIXED_DELTA_TIME, 
        &player->body.velocity
    );
    vector3AddScaled(&transform->position, &player->body.velocity, FIXED_DELTA_TIME, &transform->position);

    collisionObjectQueryScene(&player->collisionObject, &gCollisionScene, player, playerHandleCollision);

    struct RaycastHit hit;
    struct Vector3 down;
    vector3Scale(&gUp, &down, -1.0f);
    if (collisionSceneRaycast(&gCollisionScene, &player->body.transform.position, &down, PLAYER_HEAD_HEIGHT, 1, &hit)) {
        vector3AddScaled(&hit.at, &gUp, PLAYER_HEAD_HEIGHT, &player->body.transform.position);

        player->body.velocity.y = 0.0f;
    }

    rigidBodyCheckPortals(&player->body);

    float targetYaw = 0.0f;
    float targetPitch = 0.0f;

    if (controllerGetButton(0, L_CBUTTONS)) {
        targetYaw += ROTATE_RATE;
    } else if (controllerGetButton(0, R_CBUTTONS)) {
        targetYaw -= ROTATE_RATE;
    }

    if (controllerGetButton(0, U_CBUTTONS)) {
        targetPitch += ROTATE_RATE;
    } else if (controllerGetButton(0, D_CBUTTONS)) {
        targetPitch -= ROTATE_RATE;
    }

    player->yawVelocity = mathfMoveTowards(
        player->yawVelocity, 
        targetYaw, 
        player->yawVelocity * targetYaw > 0.0f ? ROTATE_RATE_DELTA : ROTATE_RATE_STOP_DELTA
    );
    player->pitchVelocity = mathfMoveTowards(
        player->pitchVelocity, 
        targetPitch, 
        player->pitchVelocity * targetPitch > 0.0f ? ROTATE_RATE_DELTA : ROTATE_RATE_STOP_DELTA
    );

    struct Quaternion deltaRotate;
    quatAxisAngle(&gUp, player->yawVelocity * FIXED_DELTA_TIME, &deltaRotate);

    struct Quaternion tempRotation;
    quatMultiply(&deltaRotate, &player->body.transform.rotation, &tempRotation);

    quatAxisAngle(&gRight, player->pitchVelocity * FIXED_DELTA_TIME, &deltaRotate);

    quatMultiply(&tempRotation, &deltaRotate, &player->body.transform.rotation);

    cameraTransform->rotation = player->body.transform.rotation;

    transformPoint(transform, &gCameraOffset, &cameraTransform->position);

    playerUpdateGrabbedObject(player);
}

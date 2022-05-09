
#include "player.h"
#include "../controls/controller.h"
#include "../util/time.h"
#include "../defs.h"
#include "../physics/point_constraint.h"
#include "../math/mathf.h"
#include "physics/collision_sphere.h"
#include "physics/collision_scene.h"
#include "physics/config.h"

#define GRAB_RAYCAST_DISTANCE   3.5f

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
    player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
    player->grabbing = NULL;
    player->pitchVelocity = 0.0f;
    player->yawVelocity = 0.0f;
    player->flags = 0;
}

#define PLAYER_SPEED    (5.0f)
#define PLAYER_ACCEL    (40.0f)
#define PLAYER_STOP_ACCEL    (80.0f)

#define ROTATE_RATE     (M_PI * 2.0f)
#define ROTATE_RATE_DELTA     (M_PI * 0.25f)
#define ROTATE_RATE_STOP_DELTA (M_PI * 0.25f)

#define JUMP_IMPULSE   3.2f

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
    if (controllerGetButtonDown(0, B_BUTTON)) {
        if (player->grabbing) {
            player->grabbing = NULL;
        } else {
            struct Ray ray;

            ray.origin = player->body.transform.position;
            quatMultVector(&player->body.transform.rotation, &gForward, &ray.dir);
            vector3Negate(&ray.dir, &ray.dir);
            
            struct RaycastHit hit;

            if (collisionSceneRaycast(&gCollisionScene, &ray, GRAB_RAYCAST_DISTANCE, 1, &hit) && hit.object->body && (hit.object->body->flags & RigidBodyFlagsGrabbable)) {
                player->grabbing = hit.object->body;

                if (hit.throughPortal) {
                    player->grabbingThroughPortal = hit.throughPortal == gCollisionScene.portalTransforms[0] ? 0 : 1;
                } else {
                    player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
                }
            }
        }
    }

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

    if (forward.y > 0.7f) {
        quatMultVector(&transform->rotation, &gUp, &forward);
        vector3Negate(&forward, &forward);
    } else if (forward.y < -0.7f) {
        quatMultVector(&transform->rotation, &gUp, &forward);
    }

    forward.y = 0.0f;
    right.y = 0.0f;

    vector3Normalize(&gForward, &gForward);
    vector3Normalize(&gRight, &gRight);


    if ((player->flags & PlayerFlagsGrounded) && controllerGetButtonDown(0, A_BUTTON)) {
        player->body.velocity.y = JUMP_IMPULSE;
    }

    OSContPad* controllerInput = controllersGetControllerData(0);

    struct Vector3 targetVelocity;

    vector3Scale(&forward, &targetVelocity, -controllerInput->stick_y * PLAYER_SPEED / 80.0f);
    vector3AddScaled(&targetVelocity, &right, controllerInput->stick_x * PLAYER_SPEED / 80.0f, &targetVelocity);

    targetVelocity.y = player->body.velocity.y;

    vector3MoveTowards(
        &player->body.velocity, 
        &targetVelocity, 
        vector3Dot(&player->body.velocity, &targetVelocity) > 0.0f ? PLAYER_ACCEL * FIXED_DELTA_TIME : PLAYER_STOP_ACCEL * FIXED_DELTA_TIME, 
        &player->body.velocity
    );

    player->body.velocity.y += GRAVITY_CONSTANT * FIXED_DELTA_TIME;

    vector3AddScaled(&transform->position, &player->body.velocity, FIXED_DELTA_TIME, &transform->position);

    collisionObjectQueryScene(&player->collisionObject, &gCollisionScene, player, playerHandleCollision);

    struct RaycastHit hit;
    struct Ray ray;
    ray.origin = player->body.transform.position;
    vector3Scale(&gUp, &ray.dir, -1.0f);
    if (collisionSceneRaycast(&gCollisionScene, &ray, PLAYER_HEAD_HEIGHT, 1, &hit)) {
        vector3AddScaled(&hit.at, &gUp, PLAYER_HEAD_HEIGHT, &player->body.transform.position);

        player->body.velocity.y = 0.0f;
        player->flags |= PlayerFlagsGrounded;
    } else {
        player->flags &= ~PlayerFlagsGrounded;
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

    struct Vector3 lookingForward;
    vector3Negate(&gForward, &lookingForward);
    quatMultVector(&player->body.transform.rotation, &lookingForward, &lookingForward);

    // if player is looking close to directly up or down, don't correct the rotation
    if (fabsf(lookingForward.y) < 0.99f) {
        struct Quaternion upRotation;
        quatLook(&lookingForward, &gUp, &upRotation);
        quatLerp(&upRotation, &player->body.transform.rotation, 0.9f, &player->body.transform.rotation);
    }
    

    // yaw
    struct Quaternion deltaRotate;
    quatAxisAngle(&gUp, player->yawVelocity * FIXED_DELTA_TIME, &deltaRotate);
    struct Quaternion tempRotation;
    quatMultiply(&deltaRotate, &player->body.transform.rotation, &tempRotation);

    // pitch
    quatAxisAngle(&gRight, player->pitchVelocity * FIXED_DELTA_TIME, &deltaRotate);
    quatMultiply(&tempRotation, &deltaRotate, &player->body.transform.rotation);

    // prevent player from looking too far up or down
    vector3Negate(&gForward, &lookingForward);
    quatMultVector(&tempRotation, &lookingForward, &lookingForward);
    struct Vector3 newLookingForward;
    vector3Negate(&gForward, &newLookingForward);
    quatMultVector(&player->body.transform.rotation, &newLookingForward, &newLookingForward);

    float pitchSign = signf(player->pitchVelocity);

    if (lookingForward.y * pitchSign > newLookingForward.y * pitchSign) {
        struct Vector3 newForward = gZeroVec;
        newForward.y = pitchSign;
        struct Vector3 newUp;
        quatMultVector(&tempRotation, &gUp, &newUp);
        quatLook(&newForward, &newUp, &player->body.transform.rotation);
        player->pitchVelocity = 0.0f;
    }

    cameraTransform->rotation = player->body.transform.rotation;
    transformPoint(transform, &gCameraOffset, &cameraTransform->position);
    playerUpdateGrabbedObject(player);
}

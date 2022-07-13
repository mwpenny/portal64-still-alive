
#include "player.h"
#include "../audio/clips.h"
#include "../audio/soundplayer.h"
#include "../controls/controller.h"
#include "../defs.h"
#include "../levels/levels.h"
#include "../math/mathf.h"
#include "../physics/collision_cylinder.h"
#include "../physics/collision_scene.h"
#include "../physics/collision.h"
#include "../physics/config.h"
#include "../physics/point_constraint.h"
#include "../util/time.h"
#include "../physics/contact_insertion.h"

#define GRAB_RAYCAST_DISTANCE   2.5f

#define PLAYER_COLLISION_LAYERS (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_FIZZLER)

struct Vector3 gGrabDistance = {0.0f, 0.0f, -1.5f};
struct Vector3 gCameraOffset = {0.0f, 0.0f, 0.0f};

struct Vector2 gPlayerColliderEdgeVectors[] = {
    {0.0f, 1.0f},
    {0.707f, 0.707f},
    {1.0f, 0.0f},
    {0.707f, -0.707f},
};

struct CollisionQuad gPlayerColliderFaces[8];

struct CollisionCylinder gPlayerCollider = {
    0.25f,
    0.5f,
    gPlayerColliderEdgeVectors,
    sizeof(gPlayerColliderEdgeVectors) / sizeof(*gPlayerColliderEdgeVectors),
    gPlayerColliderFaces,
};

struct ColliderTypeData gPlayerColliderData = {
    CollisionShapeTypeCylinder,
    &gPlayerCollider,
    0.0f,
    0.6f,
    &gCollisionCylinderCallbacks,
};

void playerInit(struct Player* player, struct Location* startLocation) {
    collisionObjectInit(&player->collisionObject, &gPlayerColliderData, &player->body, 1.0f, PLAYER_COLLISION_LAYERS);
    rigidBodyMarkKinematic(&player->body);
    player->body.flags |= RigidBodyGenerateContacts;
    collisionSceneAddDynamicObject(&player->collisionObject);

    player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
    player->grabbing = NULL;
    player->pitchVelocity = 0.0f;
    player->yawVelocity = 0.0f;
    player->flags = 0;

    if (startLocation) {
        player->lookTransform = startLocation->transform;
        player->body.currentRoom = startLocation->roomIndex;
    } else {
        transformInitIdentity(&player->lookTransform);
        player->body.currentRoom = 0;
    }
    player->lookTransform.position.y += PLAYER_HEAD_HEIGHT;
    player->body.transform = player->lookTransform;

    collisionObjectUpdateBB(&player->collisionObject);
}

#define PLAYER_SPEED    (3.0f)
#define PLAYER_ACCEL    (30.0f)
#define PLAYER_STOP_ACCEL    (80.0f)

#define ROTATE_RATE     (M_PI * 2.0f)
#define ROTATE_RATE_DELTA     (M_PI * 0.125f)
#define ROTATE_RATE_STOP_DELTA (M_PI * 0.25f)

#define JUMP_IMPULSE   3.2f

void playerHandleCollision(struct Player* player) {
    struct ContactManifold* contact = contactSolverNextManifold(&gContactSolver, &player->collisionObject, NULL);

    while (contact) {
        float offset = 0.0f;

        for (int i = 0; i < contact->contactCount; ++i) {
            struct ContactPoint* contactPoint = &contact->contacts[i];
            offset = MIN(offset, contactPoint->penetration);
        }
        
        if (offset != 0.0f) {
            vector3AddScaled(
                &player->body.transform.position, 
                &contact->normal, 
                (contact->shapeA == &player->collisionObject ? offset : -offset) * 0.95f, 
                &player->body.transform.position
            );

            float relativeVelocity = vector3Dot(&contact->normal, &player->body.velocity);

            if ((contact->shapeA == &player->collisionObject) == (relativeVelocity > 0.0f)) {
                vector3ProjectPlane(&player->body.velocity, &contact->normal, &player->body.velocity);
            }
        }

        contact = contactSolverNextManifold(&gContactSolver, &player->collisionObject, contact);
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

            ray.origin = player->lookTransform.position;
            quatMultVector(&player->lookTransform.rotation, &gForward, &ray.dir);
            vector3Negate(&ray.dir, &ray.dir);
            
            struct RaycastHit hit;

            player->collisionObject.collisionLayers = 0;

            if (collisionSceneRaycast(&gCollisionScene, player->body.currentRoom, &ray, COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_TANGIBLE, GRAB_RAYCAST_DISTANCE, 1, &hit) && hit.object->body && (hit.object->body->flags & RigidBodyFlagsGrabbable)) {
                player->grabbing = hit.object;

                if (hit.throughPortal) {
                    player->grabbingThroughPortal = hit.throughPortal == gCollisionScene.portalTransforms[0] ? 0 : 1;
                } else {
                    player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
                }
            }

            player->collisionObject.collisionLayers = PLAYER_COLLISION_LAYERS;
        }
    }

    if (player->grabbing && (player->grabbing->body->flags & RigidBodyFlagsGrabbable) == 0) {
        player->grabbing = NULL;
    }

    if (player->grabbing) {
        if (player->body.flags & RigidBodyFlagsCrossedPortal0) {
            playerApplyPortalGrab(player, 1);
        }

        if (player->body.flags & RigidBodyFlagsCrossedPortal1) {
            playerApplyPortalGrab(player, 0);
        }

        if (player->grabbing->body->flags & RigidBodyFlagsCrossedPortal0) {
            playerApplyPortalGrab(player, 0);
        }

        if (player->grabbing->body->flags & RigidBodyFlagsCrossedPortal1) {
            playerApplyPortalGrab(player, 1);
        }

        struct Vector3 grabPoint;
        struct Quaternion grabRotation = player->lookTransform.rotation;

        transformPoint(&player->lookTransform, &gGrabDistance, &grabPoint);

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

        pointConstraintMoveToPoint(player->grabbing, &grabPoint, 8.0f);
        pointConstraintRotateTo(player->grabbing->body, &grabRotation, 5.0f);
    }
}

#define DEADZONE_SIZE       5
#define MAX_JOYSTICK_RANGE  80

float playerCleanupStickInput(s8 input) {
    if (input > -DEADZONE_SIZE && input < DEADZONE_SIZE) {
        return 0.0f;
    }

    if (input >= MAX_JOYSTICK_RANGE) {
        return 1.0f;
    }

    if (input <= -MAX_JOYSTICK_RANGE) {
        return -1.0f;
    }

    return ((float)input + (input > 0 ? -DEADZONE_SIZE : DEADZONE_SIZE)) * (1.0f / (MAX_JOYSTICK_RANGE - DEADZONE_SIZE));
}

void playerUpdate(struct Player* player, struct Transform* cameraTransform) {
    struct Vector3 forward;
    struct Vector3 right;

    int doorwayMask = worldCheckDoorwaySides(&gCurrentLevel->world, &player->lookTransform.position, player->body.currentRoom);

    struct Transform* transform = &player->lookTransform;

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

    struct Vector3 targetVelocity = gZeroVec;

    if (controllerGetButton(0, L_CBUTTONS | L_JPAD)) {
        vector3AddScaled(&targetVelocity, &right, -PLAYER_SPEED, &targetVelocity);
    } else if (controllerGetButton(0, R_CBUTTONS | R_JPAD)) {
        vector3AddScaled(&targetVelocity, &right, PLAYER_SPEED, &targetVelocity);
    }

    if (controllerGetButton(0, U_CBUTTONS | U_JPAD)) {
        vector3AddScaled(&targetVelocity, &forward, -PLAYER_SPEED, &targetVelocity);
    } else if (controllerGetButton(0, D_CBUTTONS | D_JPAD)) {
        vector3AddScaled(&targetVelocity, &forward, PLAYER_SPEED, &targetVelocity);
    }
    
    targetVelocity.y = player->body.velocity.y;

    vector3MoveTowards(
        &player->body.velocity, 
        &targetVelocity, 
        vector3Dot(&player->body.velocity, &targetVelocity) > 0.0f ? PLAYER_ACCEL * FIXED_DELTA_TIME : PLAYER_STOP_ACCEL * FIXED_DELTA_TIME, 
        &player->body.velocity
    );
    player->body.angularVelocity = gZeroVec;

    player->body.velocity.y += GRAVITY_CONSTANT * FIXED_DELTA_TIME;

    vector3AddScaled(&player->body.transform.position, &player->body.velocity, FIXED_DELTA_TIME, &player->body.transform.position);

    struct RaycastHit hit;
    struct Ray ray;
    ray.origin = player->body.transform.position;
    vector3Scale(&gUp, &ray.dir, -1.0f);
    if (collisionSceneRaycast(&gCollisionScene, player->body.currentRoom, &ray, COLLISION_LAYERS_TANGIBLE, PLAYER_HEAD_HEIGHT + 0.2f, 1, &hit)) {
        struct ContactManifold* collisionManifold = contactSolverGetContactManifold(&gContactSolver, &player->collisionObject, hit.object);

        struct EpaResult newContact;

        newContact.id = 0xFFFF;

        struct Vector3* playerContact;
        struct Vector3* otherContact;
        
        if (collisionManifold->shapeA == &player->collisionObject) {
            playerContact = &newContact.contactA;
            otherContact = &newContact.contactB;
            vector3Negate(&hit.normal, &newContact.normal);
        } else {
            playerContact = &newContact.contactB;
            otherContact = &newContact.contactA;
            newContact.normal = hit.normal;
        }

        playerContact->x = 0.0f; playerContact->y = -PLAYER_HEAD_HEIGHT; playerContact->z = 0.0f;
        *otherContact = hit.at;

        newContact.penetration = hit.distance - PLAYER_HEAD_HEIGHT;

        if (hit.object && hit.object->body) {
            transformPointInverseNoScale(&hit.object->body->transform, otherContact, otherContact);
        }

        collisionManifold->friction = MAX(player->collisionObject.collider->friction, hit.object->collider->friction);
        collisionManifold->restitution = MIN(player->collisionObject.collider->bounce, hit.object->collider->bounce);

        contactInsert(collisionManifold, &newContact);
        player->flags |= PlayerFlagsGrounded;
    } else {
        player->flags &= ~PlayerFlagsGrounded;

        struct ContactManifold* manifold = contactSolverNextManifold(&gContactSolver, &player->collisionObject, NULL);

        while (manifold) {
            for (int contact = 0; contact < manifold->contactCount; ++contact) {
                if (manifold->contacts[contact].id == 0xFFFF) {
                    manifold->contactCount = 0;
                    break;
                }
            }

            manifold = contactSolverNextManifold(&gContactSolver, &player->collisionObject, manifold);
        }
    }

    playerHandleCollision(player);

    player->body.transform.rotation = player->lookTransform.rotation;    

    int didPassThroughPortal = rigidBodyCheckPortals(&player->body);

    player->lookTransform.position = player->body.transform.position;
    player->lookTransform.rotation = player->body.transform.rotation;
    quatIdent(&player->body.transform.rotation);

    if (didPassThroughPortal) {
        soundPlayerPlay(soundsPortalEnter[didPassThroughPortal - 1], 0.75f, 1.0f, NULL);
        soundPlayerPlay(soundsPortalExit[2 - didPassThroughPortal], 0.75f, 1.0f, NULL);
    }

    OSContPad* controllerInput = controllersGetControllerData(0);
    float targetYaw = -playerCleanupStickInput(controllerInput->stick_x) * ROTATE_RATE;
    float targetPitch = playerCleanupStickInput(controllerInput->stick_y) * ROTATE_RATE;

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
    quatMultVector(&player->lookTransform.rotation, &lookingForward, &lookingForward);

    // if player is looking close to directly up or down, don't correct the rotation
    if (fabsf(lookingForward.y) < 0.99f) {
        struct Quaternion upRotation;
        quatLook(&lookingForward, &gUp, &upRotation);
        quatLerp(&upRotation, &player->lookTransform.rotation, 0.9f, &player->lookTransform.rotation);
    }
    

    // yaw
    struct Quaternion deltaRotate;
    quatAxisAngle(&gUp, player->yawVelocity * FIXED_DELTA_TIME, &deltaRotate);
    struct Quaternion tempRotation;
    quatMultiply(&deltaRotate, &player->lookTransform.rotation, &tempRotation);

    // pitch
    quatAxisAngle(&gRight, player->pitchVelocity * FIXED_DELTA_TIME, &deltaRotate);
    quatMultiply(&tempRotation, &deltaRotate, &player->lookTransform.rotation);

    // prevent player from looking too far up or down
    vector3Negate(&gForward, &lookingForward);
    quatMultVector(&tempRotation, &lookingForward, &lookingForward);
    struct Vector3 newLookingForward;
    vector3Negate(&gForward, &newLookingForward);
    quatMultVector(&player->lookTransform.rotation, &newLookingForward, &newLookingForward);

    float pitchSign = signf(player->pitchVelocity);

    if (!didPassThroughPortal && lookingForward.y * pitchSign > newLookingForward.y * pitchSign) {
        struct Vector3 newForward = gZeroVec;
        newForward.y = pitchSign;
        struct Vector3 newUp;
        quatMultVector(&tempRotation, &gUp, &newUp);
        quatLook(&newForward, &newUp, &player->lookTransform.rotation);
        player->pitchVelocity = 0.0f;
    }

    cameraTransform->rotation = player->lookTransform.rotation;
    cameraTransform->position = player->lookTransform.position;
    playerUpdateGrabbedObject(player);

    collisionObjectUpdateBB(&player->collisionObject);

    player->body.currentRoom = worldCheckDoorwayCrossings(&gCurrentLevel->world, &player->lookTransform.position, player->body.currentRoom, doorwayMask);
}

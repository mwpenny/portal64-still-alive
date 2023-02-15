
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
#include "../scene/ball.h"
#include "../levels/savefile.h"

#include "../build/assets/models/player/chell.h"
#include "../build/assets/materials/static.h"

#define GRAB_RAYCAST_DISTANCE   2.5f
#define DROWN_TIME              2.0f

#define DEAD_OFFSET -0.4f

#define PLAYER_COLLISION_LAYERS (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_BALL)

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

void playerRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Player* player = (struct Player*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    struct Transform finalPlayerTransform;

    struct Vector3 forwardVector;
    struct Vector3 unusedRight;

    playerGetMoveBasis(&player->lookTransform, &forwardVector, &unusedRight);
    
    finalPlayerTransform.position = player->body.transform.position;
    quatLook(&forwardVector, &gUp, &finalPlayerTransform.rotation);
    finalPlayerTransform.scale = gOneVec;
    
    finalPlayerTransform.position.y -= PLAYER_HEAD_HEIGHT;

    transformToMatrixL(&finalPlayerTransform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, PLAYER_CHELL_DEFAULT_BONES_COUNT);

    if (!armature) {
        return;
    }

    skCalculateTransforms(&player->armature, armature);

    dynamicRenderListAddData(
        renderList,
        player_chell_model_gfx,
        matrix,
        DEFAULT_INDEX,
        &player->body.transform.position,
        armature
    );
}

void playerInit(struct Player* player, struct Location* startLocation, struct Vector3* velocity) {
    collisionObjectInit(&player->collisionObject, &gPlayerColliderData, &player->body, 1.0f, PLAYER_COLLISION_LAYERS);
    // rigidBodyMarkKinematic(&player->body);
    player->body.flags |= RigidBodyIsKinematic | RigidBodyIsPlayer;
    collisionSceneAddDynamicObject(&player->collisionObject);

    skArmatureInit(&player->armature, &player_chell_armature);
    skBlenderInit(&player->animator, player_chell_armature.numberOfBones);

    skAnimatorRunClip(&player->animator.from, &player_chell_Armature_runn_clip, 0.0f, SKAnimatorFlagsLoop);
    skAnimatorRunClip(&player->animator.to, &player_chell_Armature_runc_clip, 0.0f, SKAnimatorFlagsLoop);

    player->body.velocity = *velocity;
    player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
    player->grabConstraint.object = NULL;
    player->pitchVelocity = 0.0f;
    player->yawVelocity = 0.0f;
    player->flags = 0;

    int saveFlags = savefileReadFlags(SavefileFlagsFirstPortalGun | SavefileFlagsSecondPortalGun);

    if (saveFlags & SavefileFlagsFirstPortalGun) {
        player->flags |= PlayerHasFirstPortalGun;
    }

    if (saveFlags & SavefileFlagsSecondPortalGun) {
        player->flags |= PlayerHasSecondPortalGun;
    }

    player->dynamicId = dynamicSceneAdd(player, playerRender, &player->body.transform.position, 1.5f);
    dynamicSceneSetFlags(player->dynamicId, DYNAMIC_SCENE_OBJECT_SKIP_ROOT);

    if (startLocation) {
        player->lookTransform = startLocation->transform;
        player->body.currentRoom = startLocation->roomIndex;
    } else {
        transformInitIdentity(&player->lookTransform);
        player->body.currentRoom = 0;
    }
    player->body.transform = player->lookTransform;

    player->anchoredTo = NULL;

    collisionObjectUpdateBB(&player->collisionObject);

    dynamicSceneSetRoomFlags(player->dynamicId, ROOM_FLAG_FROM_INDEX(player->body.currentRoom));
}

#define PLAYER_SPEED    (150.0f / 64.0f)
#define PLAYER_ACCEL    (5.875f)
#define PLAYER_STOP_ACCEL    (5.875f)

#define ROTATE_RATE     (M_PI * 2.0f)
#define ROTATE_RATE_DELTA     (M_PI * 0.125f)
#define ROTATE_RATE_STOP_DELTA (M_PI * 0.25f)

#define JUMP_IMPULSE   2.7f

void playerHandleCollision(struct Player* player) {
    for (struct ContactManifold* contact = contactSolverNextManifold(&gContactSolver, &player->collisionObject, NULL);
        contact;
        contact = contactSolverNextManifold(&gContactSolver, &player->collisionObject, contact)
    ) {
        float offset = 0.0f;

        for (int i = 0; i < contact->contactCount; ++i) {
            struct ContactPoint* contactPoint = &contact->contacts[i];
            offset = MIN(offset, contactPoint->penetration);
        }

        if (contact->shapeA == player->grabConstraint.object || contact->shapeB == player->grabConstraint.object) {
            // objects being grabbed by the player shouldn't push the player
            continue;
        }
        
        if (offset != 0.0f) {
            vector3AddScaled(
                &player->body.transform.position, 
                &contact->normal, 
                (contact->shapeA == &player->collisionObject ? offset : -offset) * 0.95f, 
                &player->body.transform.position
            );
        }


        float relativeVelocity = vector3Dot(&contact->normal, &player->body.velocity);

        if ((contact->shapeA == &player->collisionObject) == (relativeVelocity > 0.0f)) {
            vector3ProjectPlane(&player->body.velocity, &contact->normal, &player->body.velocity);
        }

        if (((isColliderForBall(contact->shapeA) || isColliderForBall(contact->shapeB)) && !playerIsDead(player))) {
            playerKill(player, 0);
            soundPlayerPlay(soundsBallKill, 1.0f, 1.0f, NULL, NULL);
        }
    }
}

void playerApplyPortalGrab(struct Player* player, int portalIndex) {
    if (player->grabbingThroughPortal == PLAYER_GRABBING_THROUGH_NOTHING) {
        player->grabbingThroughPortal = portalIndex;
    } else if (player->grabbingThroughPortal != portalIndex) {
        player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
    }
}

void playerSetGrabbing(struct Player* player, struct CollisionObject* grabbing) {
    if (grabbing && !player->grabConstraint.object) {
        pointConstraintInit(&player->grabConstraint, grabbing, 8.0f, 5.0f);
        contactSolverAddPointConstraint(&gContactSolver, &player->grabConstraint);
    } else if (!grabbing && player->grabConstraint.object) {
        player->grabConstraint.object = NULL;
        contactSolverRemovePointConstraint(&gContactSolver, &player->grabConstraint);
    } else if (grabbing != player->grabConstraint.object) {
        pointConstraintInit(&player->grabConstraint, grabbing, 8.0f, 5.0f);
    }
}

void playerSignalPortalChanged(struct Player* player) {
    if (player->grabbingThroughPortal != PLAYER_GRABBING_THROUGH_NOTHING) {
        playerSetGrabbing(player, NULL);
    }
}

int playerRaycastGrab(struct Player* player, struct RaycastHit* hit) {
    struct Ray ray;

    ray.origin = player->lookTransform.position;
    quatMultVector(&player->lookTransform.rotation, &gForward, &ray.dir);
    vector3Negate(&ray.dir, &ray.dir);

    player->collisionObject.collisionLayers = 0;

    int result = collisionSceneRaycast(&gCollisionScene, player->body.currentRoom, &ray, COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_TANGIBLE, GRAB_RAYCAST_DISTANCE, 1, hit);

    player->collisionObject.collisionLayers = PLAYER_COLLISION_LAYERS;

    return result;
}

void playerUpdateGrabbedObject(struct Player* player) {
    if (playerIsDead(player)) {
        return;
    }

    if (controllerGetButtonDown(0, B_BUTTON) || controllerGetButtonDown(1, U_JPAD)) {
        if (player->grabConstraint.object) {
            if (controllerGetButtonDown(1, U_JPAD)) {
                struct Vector3 forward;
                quatMultVector(&player->lookTransform.rotation, &gForward, &forward);
                vector3AddScaled(&player->grabConstraint.object->body->velocity, &forward, -50.0f, &player->grabConstraint.object->body->velocity);
            }

            playerSetGrabbing(player, NULL);
        } else {
            struct RaycastHit hit;

            if (playerRaycastGrab(player, &hit)) {
                hit.object->flags |= COLLISION_OBJECT_INTERACTED;

                if (hit.object->body && (hit.object->body->flags & RigidBodyFlagsGrabbable)) {
                    playerSetGrabbing(player, hit.object);

                    if (hit.throughPortal) {
                        player->grabbingThroughPortal = hit.throughPortal == gCollisionScene.portalTransforms[0] ? 0 : 1;
                    } else {
                        player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
                    }
                }
            }
        }
    }

    if (player->grabConstraint.object && (player->grabConstraint.object->body->flags & RigidBodyFlagsGrabbable) == 0) {
        playerSetGrabbing(player, NULL);
    }

    if (player->grabConstraint.object) {
        if (player->body.flags & RigidBodyFlagsCrossedPortal0) {
            playerApplyPortalGrab(player, 1);
        }

        if (player->body.flags & RigidBodyFlagsCrossedPortal1) {
            playerApplyPortalGrab(player, 0);
        }

        if (player->grabConstraint.object->body->flags & RigidBodyFlagsCrossedPortal0) {
            playerApplyPortalGrab(player, 0);
        }

        if (player->grabConstraint.object->body->flags & RigidBodyFlagsCrossedPortal1) {
            playerApplyPortalGrab(player, 1);
        }

        struct Vector3 grabPoint;
        struct Quaternion grabRotation = player->lookTransform.rotation;

        transformPoint(&player->lookTransform, &gGrabDistance, &grabPoint);

        if (player->grabbingThroughPortal != PLAYER_GRABBING_THROUGH_NOTHING) {
            if (!collisionSceneIsPortalOpen()) {
                // portal was closed while holding object through it
                playerSetGrabbing(player, NULL);
                return;
            }

            struct Transform pointTransform;
            collisionSceneGetPortalTransform(player->grabbingThroughPortal, &pointTransform);

            transformPoint(&pointTransform, &grabPoint, &grabPoint);
            struct Quaternion finalRotation;
            quatMultiply(&pointTransform.rotation, &grabRotation, &finalRotation);
            grabRotation = finalRotation;
        }

        pointConstraintUpdateTarget(&player->grabConstraint, &grabPoint, &grabRotation);
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

void playerGetMoveBasis(struct Transform* transform, struct Vector3* forward, struct Vector3* right) {
    quatMultVector(&transform->rotation, &gForward, forward);
    quatMultVector(&transform->rotation, &gRight, right);

    if (forward->y > 0.7f) {
        quatMultVector(&transform->rotation, &gUp, forward);
        vector3Negate(forward, forward);
    } else if (forward->y < -0.7f) {
        quatMultVector(&transform->rotation, &gUp, forward);
    }

    forward->y = 0.0f;
    right->y = 0.0f;

    vector3Normalize(forward, forward);
    vector3Normalize(right, right);
}

void playerGivePortalGun(struct Player* player, int flags) {
    player->flags |= flags;

    if (flags & PlayerHasFirstPortalGun) {
        savefileSetFlags(SavefileFlagsFirstPortalGun);
    }

    if (flags & PlayerHasSecondPortalGun) {
        savefileSetFlags(SavefileFlagsSecondPortalGun);
    }
}

void playerKill(struct Player* player, int isUnderwater) {
    if (isUnderwater) {
        player->flags |= PlayerIsUnderwater;
        player->drownTimer = DROWN_TIME;
        return;
    }

    player->flags |= PlayerIsDead;
    // drop the portal gun
    player->flags &= ~(PlayerHasFirstPortalGun | PlayerHasSecondPortalGun);
    playerSetGrabbing(player, NULL);
}

int playerIsDead(struct Player* player) {
    return (player->flags & PlayerIsDead) != 0;
}

struct SKAnimationClip* playerDetermineNextClip(struct Player* player, float* blendLerp, float* startTime, struct Vector3* forwardDir, struct Vector3* rightDir) {
    float horzSpeed = player->body.velocity.x * player->body.velocity.x + player->body.velocity.z * player->body.velocity.z;

    if (horzSpeed < 0.0001f) {
        *blendLerp = 0.0f;
        *startTime = 0.0f;
        return &player_chell_Armature_idle_clip;
    }

    horzSpeed = sqrtf(horzSpeed);

    *blendLerp = 1.0f - horzSpeed * (1.0f / PLAYER_SPEED);

    if (*blendLerp < 0.0f) {
        *blendLerp = 0.0f;
    }

    if (*blendLerp > 1.0f) {
        *blendLerp = 1.0f;
    }

    *startTime = player->animator.from.currentTime;

    float forward = forwardDir->x * player->body.velocity.x + forwardDir->z * player->body.velocity.z;
    float right = rightDir->x * player->body.velocity.x + rightDir->z * player->body.velocity.z;

    if (fabsf(forward) > fabsf(right)) {
        if (forward > 0.0f) {
            return &player_chell_Armature_runs_clip;
        } else {
            return &player_chell_Armature_runn_clip;
        }
    } else {
        if (right > 0.0f) {
            return &player_chell_Armature_rune_clip;
        } else {
            return &player_chell_Armature_runw_clip;
        }
    }

}

void playerUpdate(struct Player* player, struct Transform* cameraTransform) {
    struct Vector3 forward;
    struct Vector3 right;

    skBlenderUpdate(&player->animator, player->armature.pose, FIXED_DELTA_TIME);

    int doorwayMask = worldCheckDoorwaySides(&gCurrentLevel->world, &player->lookTransform.position, player->body.currentRoom);
    playerGetMoveBasis(&player->lookTransform, &forward, &right);

    int isDead = playerIsDead(player);

    if (!isDead && (player->flags & PlayerFlagsGrounded) && controllerGetButtonDown(0, A_BUTTON)) {
        player->body.velocity.y = JUMP_IMPULSE;
    }

    struct Vector3 targetVelocity = gZeroVec;

    if (!isDead) {
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

    struct Vector3 prevPos = player->body.transform.position;

    vector3AddScaled(&player->body.transform.position, &player->body.velocity, FIXED_DELTA_TIME, &player->body.transform.position);

    if (player->anchoredTo) {
        struct Vector3 newAnchor;
        transformPoint(&player->anchoredTo->transform, &player->relativeAnchor, &newAnchor);
        vector3Add(&player->body.transform.position, &newAnchor, &player->body.transform.position);
        vector3Sub(&player->body.transform.position, &player->lastAnchorPoint, &player->body.transform.position);
    }

    struct Box3D sweptBB = player->collisionObject.boundingBox;
    collisionObjectUpdateBB(&player->collisionObject);
    box3DUnion(&sweptBB, &player->collisionObject.boundingBox, &sweptBB);

    collisionObjectCollideMixed(&player->collisionObject, &prevPos, &sweptBB, &gCollisionScene, &gContactSolver);

    player->anchoredTo = NULL;

    struct RaycastHit hit;
    struct Ray ray;
    ray.origin = player->body.transform.position;
    vector3Scale(&gUp, &ray.dir, -1.0f);
    if (collisionSceneRaycast(&gCollisionScene, player->body.currentRoom, &ray, COLLISION_LAYERS_TANGIBLE, PLAYER_HEAD_HEIGHT + 0.2f, 1, &hit)) {
        float penetration = hit.distance - PLAYER_HEAD_HEIGHT;

        if (penetration < 0.0f) {
            vector3AddScaled(&player->body.transform.position, &gUp, -penetration, &player->body.transform.position);
            if (player->body.velocity.y < 0.0f) {
                player->body.velocity.y = 0.0f;
            }
        }

        hit.object->flags |= COLLISION_OBJECT_PLAYER_STANDING;
        player->flags |= PlayerFlagsGrounded;

        if (hit.object == player->grabConstraint.object) {
            playerSetGrabbing(player, NULL);
        }

        if (hit.object->body && (hit.object->body->flags & RigidBodyIsKinematic)) {
            player->anchoredTo = hit.object->body;
            player->lastAnchorPoint = hit.at;
            transformPointInverseNoScale(&player->anchoredTo->transform, &hit.at, &player->relativeAnchor);
        }
    } else {
        player->flags &= ~PlayerFlagsGrounded;
    }
    
    struct ContactManifold* manifold = contactSolverNextManifold(&gContactSolver, &player->collisionObject, NULL);

    while (manifold) {
        contactSolverCleanupManifold(manifold);
        manifold = contactSolverNextManifold(&gContactSolver, &player->collisionObject, manifold);
    }

    collisionObjectUpdateBB(&player->collisionObject);
    playerHandleCollision(player);

    player->body.transform.rotation = player->lookTransform.rotation;    

    int didPassThroughPortal = rigidBodyCheckPortals(&player->body);

    player->lookTransform.position = player->body.transform.position;
    player->lookTransform.rotation = player->body.transform.rotation;
    quatIdent(&player->body.transform.rotation);

    if (didPassThroughPortal) {
        soundPlayerPlay(soundsPortalEnter[didPassThroughPortal - 1], 0.75f, 1.0f, NULL, NULL);
        soundPlayerPlay(soundsPortalExit[2 - didPassThroughPortal], 0.75f, 1.0f, NULL, NULL);
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

    if (player->flags & PlayerIsDead) {
        cameraTransform->position.y += DEAD_OFFSET;
    }

    player->body.currentRoom = worldCheckDoorwayCrossings(&gCurrentLevel->world, &player->lookTransform.position, player->body.currentRoom, doorwayMask);

    dynamicSceneSetRoomFlags(player->dynamicId, ROOM_FLAG_FROM_INDEX(player->body.currentRoom));

    float startTime = 0.0f;
    struct SKAnimationClip* clip = playerDetermineNextClip(player, &player->animator.blendLerp, &startTime, &forward, &right);

    if (clip != player->animator.from.currentClip) {
        skAnimatorRunClip(&player->animator.from, clip, startTime, SKAnimatorFlagsLoop);
    }

    if (player->flags & PlayerIsUnderwater) {
        player->drownTimer -= FIXED_DELTA_TIME;

        if (player->drownTimer < 0.0f) {
            player->flags &= ~PlayerIsUnderwater;
            playerKill(player, 0);
        }
    }
}

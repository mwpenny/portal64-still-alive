
#include <stdlib.h>
#include "player.h"
#include "../audio/clips.h"
#include "../audio/soundplayer.h"
#include "../controls/controller_actions.h"
#include "../defs.h"
#include "../levels/levels.h"
#include "../math/mathf.h"
#include "../physics/collision_sphere.h"
#include "../physics/collision_scene.h"
#include "../physics/collision.h"
#include "../physics/config.h"
#include "../physics/point_constraint.h"
#include "../util/time.h"
#include "../physics/contact_insertion.h"
#include "../scene/ball.h"
#include "../savefile/savefile.h"

#include "../build/assets/models/player/chell.h"
#include "../build/assets/materials/static.h"

#define GRAB_RAYCAST_DISTANCE   2.5f
#define DROWN_TIME              2.0f
#define STEP_TIME               0.35f

#define STAND_SPEED             1.5f
#define SHAKE_DISTANCE          0.02f

#define DEAD_OFFSET -0.4f

#define PLAYER_COLLISION_LAYERS (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_BALL)

struct Vector3 gGrabDistance = {0.0f, 0.0f, -1.5f};
struct Vector3 gCameraOffset = {0.0f, 0.0f, 0.0f};

struct Vector3 gPortalGunOffset = {0.120957, -0.113587, -0.20916};
struct Vector3 gPortalGunShootOffset = {0.120957, -0.113587, -0.08};
struct Vector3 gPortalGunForward = {0.1f, -0.1f, 1.0f};
struct Vector3 gPortalGunShootForward = {0.3f, -0.25f, 1.0f};
struct Vector3 gPortalGunUp = {0.0f, 1.0f, 0.0f};

struct CollisionQuad gPlayerColliderFaces[8];

struct CollisionSphere gPlayerCollider = {
    0.25f,
};

struct ColliderTypeData gPlayerColliderData = {
    CollisionShapeTypeSphere,
    &gPlayerCollider,
    0.0f,
    0.6f,
    &gCollisionSphereCallbacks,
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

void playerInit(struct Player* player, struct Location* startLocation, struct Vector3* velocity, struct CollisionObject* portalGunObject) {
    player->flyingSoundLoopId = soundPlayerPlay(soundsFastFalling, 0.0f, 0.5f, NULL, NULL);

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
    player->gunConstraint.object = NULL;
    player->pitchVelocity = 0.0f;
    player->yawVelocity = 0.0f;
    player->flags = 0;
    player->stepTimer = STEP_TIME;
    player->shakeTimer = 0.0f;
    player->currentFoot = 0;

    if (gCurrentLevelIndex == 0){
        player->flags &= ~PlayerHasFirstPortalGun;
        player->flags &= ~PlayerHasSecondPortalGun;
    }

    // player->flags |= PlayerHasFirstPortalGun | PlayerHasSecondPortalGun;

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

    pointConstraintInit(&player->gunConstraint, portalGunObject, 20.0f, 2.5f, 1, 0.5f);
    contactSolverAddPointConstraint(&gContactSolver, &player->gunConstraint);
}

#define PLAYER_SPEED    (150.0f / 64.0f)
#define PLAYER_ACCEL    (5.875f)
#define PLAYER_AIR_ACCEL    (5.875f)
#define PLAYER_STOP_ACCEL    (5.875f)
#define PLAYER_SLIDE_ACCEL    (40.0f)

#define MIN_ROTATE_RATE (M_PI * 0.5f)
#define MAX_ROTATE_RATE (M_PI * 3.5f)

#define MIN_ROTATE_RATE_DELTA (M_PI * 0.06125f)
#define MAX_ROTATE_RATE_DELTA MAX_ROTATE_RATE

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

        float prevY = player->body.transform.position.y;
        float prevVelY = player->body.velocity.y;
        
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

        if (collisionObjectIsGrabbable(contact->shapeA) || collisionObjectIsGrabbable(contact->shapeB)) {
            player->body.transform.position.y = MAX(player->body.transform.position.y, prevY);
            player->body.velocity.y = MAX(player->body.velocity.y, prevVelY);
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
        pointConstraintInit(&player->grabConstraint, grabbing, 8.0f, 5.0f, 0, 1.0f);
        contactSolverAddPointConstraint(&gContactSolver, &player->grabConstraint);
    } else if (!grabbing && player->grabConstraint.object) {
        player->grabConstraint.object = NULL;
        contactSolverRemovePointConstraint(&gContactSolver, &player->grabConstraint);
    } else if (grabbing != player->grabConstraint.object) {
        pointConstraintInit(&player->grabConstraint, grabbing, 8.0f, 5.0f, 0, 1.0f);
    }
}

void playerShakeUpdate(struct Player* player) {
    if (player->shakeTimer > 0.0f){
        player->shakeTimer -= FIXED_DELTA_TIME;

        float max = SHAKE_DISTANCE;
        float min = -SHAKE_DISTANCE;
        float x;
        x = ((float)rand()/(float)(RAND_MAX));
        x = min + x * ( max - min );
        player->lookTransform.position.x += x;
        x = ((float)rand()/(float)(RAND_MAX));
        x = min + x * ( max - min );
        player->lookTransform.position.y += x;
        x = ((float)rand()/(float)(RAND_MAX));
        x = min + x * ( max - min );
        player->lookTransform.position.z += x;

        if (player->shakeTimer < 0.0f){
            player->shakeTimer = 0.0f;
        }
    }
}

void playerSignalPortalChanged(struct Player* player) {
    if (player->grabbingThroughPortal != PLAYER_GRABBING_THROUGH_NOTHING) {
        playerSetGrabbing(player, NULL);
    }
}

int playerIsGrabbing(struct Player* player) {
    return player->grabConstraint.object != NULL;
}

int playerRaycastGrab(struct Player* player, struct RaycastHit* hit, int checkPastObject) {
    struct Ray ray;

    ray.origin = player->lookTransform.position;
    quatMultVector(&player->lookTransform.rotation, &gForward, &ray.dir);
    vector3Negate(&ray.dir, &ray.dir);
    int result;

    player->collisionObject.collisionLayers = 0;

    if (checkPastObject){
        short prevCollisionLayers = player->grabConstraint.object->collisionLayers;
        player->grabConstraint.object->collisionLayers = 0;
        result = collisionSceneRaycast(&gCollisionScene, player->body.currentRoom, &ray, COLLISION_LAYERS_TANGIBLE, GRAB_RAYCAST_DISTANCE, 1, hit);
        player->grabConstraint.object->collisionLayers = prevCollisionLayers;
    }
    else{
        result = collisionSceneRaycast(&gCollisionScene, player->body.currentRoom, &ray, COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_TANGIBLE, GRAB_RAYCAST_DISTANCE, 1, hit);
    }

    player->collisionObject.collisionLayers = PLAYER_COLLISION_LAYERS;

    return result;
}

void playerUpdateGrabbedObject(struct Player* player) {
    if (playerIsDead(player)) {
        return;
    }

    if (controllerActionGet(ControllerActionUseItem)) {
        if (player->grabConstraint.object) {
            playerSetGrabbing(player, NULL);
        } else {
            struct RaycastHit hit;

            if (playerRaycastGrab(player, &hit, 0)) {
                hit.object->flags |= COLLISION_OBJECT_INTERACTED;

                if (hit.object->body && (hit.object->body->flags & RigidBodyFlagsGrabbable)) {
                    playerSetGrabbing(player, hit.object);
                    player->flags |= PlayerJustSelect;

                    if (hit.throughPortal) {
                        player->grabbingThroughPortal = hit.throughPortal == gCollisionScene.portalTransforms[0] ? 0 : 1;
                    } else {
                        player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
                    }
                }
                else if ((hit.object->body)){
                    player->flags |= PlayerJustSelect;
                }
                else{
                    player->flags |= PlayerJustDeniedSelect;
                }
            }
            else{
                player->flags |= PlayerJustDeniedSelect;
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

        // try to determine how far away to set the grab dist
        struct RaycastHit hit;
        struct Vector3 temp_grab_dist = gGrabDistance;
        if (playerRaycastGrab(player, &hit, 1)){
            float dist = hit.distance;
            temp_grab_dist.z = maxf(((-1.0f*fabsf(dist))+0.2f), gGrabDistance.z);
            temp_grab_dist.z = minf(temp_grab_dist.z, -0.2f);
        }
        //drop the object if grab distance becomes too close to player
        if (fabsf(temp_grab_dist.z) < 0.3){
            playerSetGrabbing(player, NULL);
            return;
        }

        transformPoint(&player->lookTransform, &temp_grab_dist, &grabPoint);

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

void playerUpdateGunObject(struct Player* player) {
    struct Vector3 forward;
    struct Vector3 offset;

    if (player->flags & PlayerJustShotPortalGun){
        forward = gPortalGunShootForward;
        offset = gPortalGunShootOffset;
    }else{
        forward = gPortalGunForward;
        offset = gPortalGunOffset;
    }

    struct Quaternion relativeRotation;
    quatLook(&forward, &gPortalGunUp, &relativeRotation);
    quatMultiply(&player->lookTransform.rotation, &relativeRotation, &player->gunConstraint.object->body->transform.rotation);


    struct Vector3 gunPoint;
    struct Vector3 temp_gun_dist = offset;
    transformPoint(&player->lookTransform, &temp_gun_dist, &gunPoint);

    pointConstraintUpdateTarget(&player->gunConstraint, &gunPoint, &player->gunConstraint.object->body->transform.rotation);
    
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
}

void playerUpdateSpeedSound(struct Player* player) {
    float soundPlayerVolume;
    soundPlayerVolume = sqrtf(vector3MagSqrd(&player->body.velocity))*(0.6f / MAX_PORTAL_SPEED);
    soundPlayerVolume = clampf(soundPlayerVolume, 0.0, 1.0f);
    soundPlayerAdjustVolume(player->flyingSoundLoopId, soundPlayerVolume);
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

void playerUpdate(struct Player* player) {
    struct Vector3 forward;
    struct Vector3 right;

    if (player->flags & PlayerInCutscene) {
        return;
    }

    skBlenderUpdate(&player->animator, player->armature.pose, FIXED_DELTA_TIME);

    int doorwayMask = worldCheckDoorwaySides(&gCurrentLevel->world, &player->lookTransform.position, player->body.currentRoom);
    playerGetMoveBasis(&player->lookTransform, &forward, &right);

    int isDead = playerIsDead(player);

    if (!isDead && (player->flags & PlayerFlagsGrounded) && controllerActionGet(ControllerActionJump)) {
        player->body.velocity.y = JUMP_IMPULSE;
        player->flags |= PlayerJustJumped;
    }

    struct Vector3 targetVelocity = gZeroVec;

    float camera_y_modifier = 0.0;
    if (player->flags & PlayerCrouched){
        camera_y_modifier = -0.25;
    }
    else{
        camera_y_modifier = 0.0;
    }

    if (!isDead) {
        struct Vector2 moveInput = controllerDirectionGet(ControllerActionMove);

        vector3AddScaled(&targetVelocity, &right, PLAYER_SPEED * moveInput.x, &targetVelocity);
        vector3AddScaled(&targetVelocity, &forward, -PLAYER_SPEED * moveInput.y, &targetVelocity);

        // if player isnt crouched, crouch
        if (!(player->flags & PlayerCrouched) && (controllerActionGet(ControllerActionDuck))){
            player->flags |= PlayerCrouched;
            camera_y_modifier = -0.25;
        }
        //if player crouched, uncrouch
        else if ((player->flags & PlayerCrouched) && (controllerActionGet(ControllerActionDuck))){
            player->flags &= ~PlayerCrouched;
            camera_y_modifier = 0.0;
        }

        //look straight forward
        if (controllerActionGet(ControllerActionLookForward)){
            struct Vector3 lookingForward;
            vector3Negate(&gForward, &lookingForward);
            quatMultVector(&player->lookTransform.rotation, &lookingForward, &lookingForward);
            if (fabsf(lookingForward.y) < 0.999f) {
                lookingForward.y = 0;
                quatLook(&lookingForward, &gUp, &player->lookTransform.rotation);
            }
        }
        //look behind
        if (controllerActionGet(ControllerActionLookBackward)){
            struct Vector3 lookingForward;
            vector3Negate(&gForward, &lookingForward);
            quatMultVector(&player->lookTransform.rotation, &lookingForward, &lookingForward);
            if (fabsf(lookingForward.y) < 0.999f) {
                lookingForward.y = 0;
                quatLook(&lookingForward, &gUp, &player->lookTransform.rotation);
                //look directly behind
                struct Quaternion behindRotation;
                behindRotation.x=(player->lookTransform.rotation.z);
                behindRotation.y=player->lookTransform.rotation.w;
                behindRotation.z=(-1.0f*player->lookTransform.rotation.x);
                behindRotation.w=(-1.0f*player->lookTransform.rotation.y);

                player->lookTransform.rotation = behindRotation;
            }
        }
    }

    targetVelocity.y = player->body.velocity.y;

    float velocityDot = vector3Dot(&player->body.velocity, &targetVelocity);
    int isAccelerating = velocityDot > 0.0f;
    float acceleration = 0.0f;
    float velocitySqrd = vector3MagSqrd(&player->body.velocity);
    int isFast = velocitySqrd >= PLAYER_SPEED * PLAYER_SPEED;
    playerUpdateSpeedSound(player);

    if (!(player->flags & PlayerFlagsGrounded)) {
        if (isFast) {
            struct Vector3 movementCenter;
            vector3Scale(&player->body.velocity, &movementCenter, -PLAYER_SPEED / sqrtf(velocitySqrd));
            targetVelocity.x += player->body.velocity.x + movementCenter.x;
            targetVelocity.z += player->body.velocity.z + movementCenter.z;
        }

        acceleration = PLAYER_AIR_ACCEL * FIXED_DELTA_TIME;
    } else if (isFast) {
        acceleration = PLAYER_SLIDE_ACCEL * FIXED_DELTA_TIME;
    } else if (isAccelerating) {
        acceleration = PLAYER_ACCEL * FIXED_DELTA_TIME;
    } else {
        acceleration = PLAYER_STOP_ACCEL * FIXED_DELTA_TIME;
    }

    vector3MoveTowards(
        &player->body.velocity, 
        &targetVelocity, 
        acceleration, 
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
            vector3AddScaled(&player->body.transform.position, &gUp, MIN(-penetration, STAND_SPEED * FIXED_DELTA_TIME), &player->body.transform.position);
            if (player->body.velocity.y < 0.0f) {
                player->body.velocity.y = 0.0f;
            }
        }

        hit.object->flags |= COLLISION_OBJECT_PLAYER_STANDING;
        if (!(player->flags & PlayerFlagsGrounded)){
            player->flags |= PlayerJustLanded;
        }
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
    player->lookTransform.position.y += camera_y_modifier;

    //if player is shaking, shake screen 
    playerShakeUpdate(player);

    player->lookTransform.rotation = player->body.transform.rotation;
    quatIdent(&player->body.transform.rotation);

    if (didPassThroughPortal) {
        soundPlayerPlay(soundsPortalEnter[didPassThroughPortal - 1], 0.75f, 1.0f, NULL, NULL);
        soundPlayerPlay(soundsPortalExit[2 - didPassThroughPortal], 0.75f, 1.0f, NULL, NULL);
    }

    struct Vector2 lookInput = controllerDirectionGet(ControllerActionRotate);
    float rotateRate = mathfLerp(MIN_ROTATE_RATE, MAX_ROTATE_RATE, (float)gSaveData.controls.sensitivity / 0xFFFF);
    float targetYaw = -lookInput.x * rotateRate;
    float targetPitch = lookInput.y * rotateRate;

    if (gSaveData.controls.flags & ControlSaveFlagsInvert) {
        targetYaw = -targetYaw;
        targetPitch = -targetPitch;
    }

    float rotateRateDelta = mathfLerp(MIN_ROTATE_RATE_DELTA, MAX_ROTATE_RATE_DELTA, (float)gSaveData.controls.acceleration / 0xFFFF);

    player->yawVelocity = mathfMoveTowards(
        player->yawVelocity, 
        targetYaw, 
        rotateRateDelta
    );
    player->pitchVelocity = mathfMoveTowards(
        player->pitchVelocity, 
        targetPitch, 
        rotateRateDelta
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

    playerUpdateGrabbedObject(player);
    playerUpdateGunObject(player);

    collisionObjectUpdateBB(&player->collisionObject);

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

    // player not moving on ground
    if ((player->flags & PlayerFlagsGrounded) && (player->body.velocity.x == 0) && (player->body.velocity.z == 0)){
        player->stepTimer = STEP_TIME;
        player->flags &= ~PlayerIsStepping;
    }
    // player moving on ground
    else{
        player->stepTimer -= FIXED_DELTA_TIME;

        if (player->stepTimer < 0.0f) {
            player->flags |= PlayerIsStepping;
            player->stepTimer = STEP_TIME;
            player->currentFoot = !player->currentFoot;
        }
    }

}

void playerApplyCameraTransform(struct Player* player, struct Transform* cameraTransform) {
    cameraTransform->rotation = player->lookTransform.rotation;
    cameraTransform->position = player->lookTransform.position;
    
    if (player->flags & PlayerIsDead) {
        cameraTransform->position.y += DEAD_OFFSET;
    }
}

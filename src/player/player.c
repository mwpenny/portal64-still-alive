
#include <stdlib.h>
#include "player.h"
#include "player_rumble_clips.h"
#include "../audio/clips.h"
#include "../audio/soundplayer.h"
#include "../controls/controller_actions.h"
#include "../defs.h"
#include "../levels/levels.h"
#include "../math/mathf.h"
#include "../physics/collision_capsule.h"
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
#include "../build/assets/models/portal_gun/w_portalgun.h"

#define GRAB_RAYCAST_DISTANCE   2.5f
#define DROWN_TIME              2.0f
#define STEP_TIME               0.35f

#define STAND_SPEED             1.5f
#define SHAKE_DISTANCE          0.02f

#define DEAD_OFFSET -0.4f

#define PLAYER_COLLISION_LAYERS (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_BALL)

#define FUNNEL_DAMPENING_CONSTANT 0.32f
#define FUNNEL_CENTERING_CONSTANT 0.05f
#define FUNNEL_ACCEPTABLE_CENTERED_DISTANCE 0.1f
#define FUNNEL_MAX_DIST 1.2f
#define FUNNEL_MIN_DOWN_VEL -2.25f
#define FUNNEL_MAX_HORZ_VEL 7.0f

struct Vector3 gGrabDistance = {0.0f, 0.0f, -1.5f};
struct Vector3 gCameraOffset = {0.0f, 0.0f, 0.0f};

struct CollisionQuad gPlayerColliderFaces[8];

#define TARGET_CAPSULE_EXTEND_HEIGHT   0.45f

struct CollisionCapsule gPlayerCollider = {
    0.25f,
    TARGET_CAPSULE_EXTEND_HEIGHT,
};

struct ColliderTypeData gPlayerColliderData = {
    CollisionShapeTypeSphere,
    &gPlayerCollider,
    0.0f,
    0.6f,
    &gCollisionCapsuleCallbacks,
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

    Gfx* gunAttachment = portal_gun_w_portalgun_model_gfx;
    Gfx* attachments = skBuildAttachments(&player->armature, (player->flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun)) ? &gunAttachment : NULL, renderState);

    Gfx* objectRender = renderStateAllocateDLChunk(renderState, 4);
    Gfx* dl = objectRender;

    if (attachments) {
        gSPSegment(dl++, BONE_ATTACHMENT_SEGMENT,  osVirtualToPhysical(attachments));
    }
    gSPSegment(dl++, MATRIX_TRANSFORM_SEGMENT,  osVirtualToPhysical(armature));
    gSPDisplayList(dl++, player->armature.displayList);
    gSPEndDisplayList(dl++);


    dynamicRenderListAddDataTouchingPortal(
        renderList,
        objectRender,
        matrix,
        DEFAULT_INDEX,
        &player->body.transform.position,
        armature,
        player->body.flags
    );
}

void playerInit(struct Player* player, struct Location* startLocation, struct Vector3* velocity) {
    player->flyingSoundLoopId = soundPlayerPlay(soundsFastFalling, 0.0f, 0.5f, NULL, NULL, SoundTypeAll);

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
    player->stepTimer = STEP_TIME;
    player->shakeTimer = 0.0f;
    player->currentFoot = 0;
    player->passedThroughPortal = 0;

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
    player->lastAnchorToPosition = gZeroVec;
    player->lastAnchorToVelocity = gZeroVec;

    collisionObjectUpdateBB(&player->collisionObject);

    dynamicSceneSetRoomFlags(player->dynamicId, ROOM_FLAG_FROM_INDEX(player->body.currentRoom));
}

#define PLAYER_SPEED    (150.0f / 64.0f)
#define PLAYER_ACCEL    (5.875f)
#define PLAYER_AIR_ACCEL    (1.875f)
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
            playerHandleLandingRumble(relativeVelocity);
        }

        if (collisionObjectIsGrabbable(contact->shapeA) || collisionObjectIsGrabbable(contact->shapeB)) {
            player->body.transform.position.y = MAX(player->body.transform.position.y, prevY);
            player->body.velocity.y = MAX(player->body.velocity.y, prevVelY);
        }

        if (((isColliderForBall(contact->shapeA) || isColliderForBall(contact->shapeB)) && !playerIsDead(player))) {
            playerKill(player, 0);
            soundPlayerPlay(soundsBallKill, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
        }
    }
}

void playerApplyPortalGrab(struct Player* player, int portalIndex) {
    if (portalIndex){
        player->grabbingThroughPortal -= 1;
    }else{
        player->grabbingThroughPortal += 1;
    }
}

void playerSetGrabbing(struct Player* player, struct CollisionObject* grabbing) {
    if (grabbing && grabbing->flags & COLLISION_OBJECT_PLAYER_STANDING){
        player->grabConstraint.object = NULL;
        contactSolverRemovePointConstraint(&gContactSolver, &player->grabConstraint);
        player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
    }
    else if (grabbing && !player->grabConstraint.object) {
        pointConstraintInit(&player->grabConstraint, grabbing, 8.0f, 5.0f, 1.0f);
        contactSolverAddPointConstraint(&gContactSolver, &player->grabConstraint);
        hudResolvePrompt(&gScene.hud, CutscenePromptTypePickup);
    } else if (!grabbing && player->grabConstraint.object) {
        player->grabConstraint.object = NULL;
        contactSolverRemovePointConstraint(&gContactSolver, &player->grabConstraint);
        hudResolvePrompt(&gScene.hud, CutscenePromptTypeDrop);
        player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
    } else if (grabbing != player->grabConstraint.object) {
        pointConstraintInit(&player->grabConstraint, grabbing, 8.0f, 5.0f, 1.0f);
    }
}

void playerShakeUpdate(struct Player* player) {
    if (player->shakeTimer > 0.0f){
        player->shakeTimer -= FIXED_DELTA_TIME;

        float magnitude = 1.0f;

        if (player->shakeTimer < 1.0f) {
            magnitude = player->shakeTimer;
        }

        float max = SHAKE_DISTANCE * magnitude;
        float min = -SHAKE_DISTANCE * magnitude;
        player->lookTransform.position.x += randomInRangef(min, max);
        player->lookTransform.position.y += randomInRangef(min, max);
        player->lookTransform.position.z += randomInRangef(min, max);

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

                if (hit.object->body && (hit.object->body->flags & RigidBodyFlagsGrabbable) && !(hit.object->flags & COLLISION_OBJECT_PLAYER_STANDING)) {
                    playerSetGrabbing(player, hit.object);
                    player->flags |= PlayerJustSelect;
                    player->grabbingThroughPortal = hit.numPortalsPassed;
                }
                else if ((hit.object->body)){
                    player->flags |= PlayerJustSelect;
                    hudResolvePrompt(&gScene.hud, CutscenePromptTypeUse);
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

    // if the object is being held through a portal and can no longer be seen, drop it.
    if (player->grabConstraint.object && player->grabbingThroughPortal){
        struct RaycastHit testhit;
        if (playerRaycastGrab(player, &testhit, 0)){
            if ((testhit.numPortalsPassed != player->grabbingThroughPortal) && (testhit.object != player->grabConstraint.object)){
                playerSetGrabbing(player, NULL);
                return;
            }
        }
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
            collisionSceneGetPortalTransform(player->grabbingThroughPortal > 0 ? 0 : 1, &pointTransform);

            for (int i = 0; i < abs(player->grabbingThroughPortal); ++i) {
                transformPoint(&pointTransform, &grabPoint, &grabPoint);
                struct Quaternion finalRotation;
                quatMultiply(&pointTransform.rotation, &grabRotation, &finalRotation);
                grabRotation = finalRotation;
            }
        }

        pointConstraintUpdateTarget(&player->grabConstraint, &grabPoint, &grabRotation);
    }
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
    if (soundPlayerVolume >= 0.75){
        hudShowSubtitle(&gScene.hud, PORTALPLAYER_WOOSH, SubtitleTypeCaption);
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
    rumblePakClipPlay(&gPlayerDieRumbleWave);
}

int playerIsDead(struct Player* player) {
    return (player->flags & PlayerIsDead) != 0;
}

struct SKAnimationClip* gPlayerIdleClips[] = {
    &player_chell_Armature_idle_clip,
    &player_chell_Armature_idle_portalgun_clip,
};

struct SKAnimationClip* gPlayerRunSClips[] = {
    &player_chell_Armature_runs_clip,
    &player_chell_Armature_runs_portalgun_clip,
};

struct SKAnimationClip* gPlayerRunNClips[] = {
    &player_chell_Armature_runn_clip,
    &player_chell_Armature_runn_portalgun_clip,
};

struct SKAnimationClip* gPlayerRunWClips[] = {
    &player_chell_Armature_runw_clip,
    &player_chell_Armature_runw_portalgun_clip,
};

struct SKAnimationClip* gPlayerRunEClips[] = {
    &player_chell_Armature_rune_clip,
    &player_chell_Armature_rune_portalgun_clip,
};

struct SKAnimationClip* gPlayerJumpClips[] = {
    &player_chell_Armature_standing_jump_clip,
    &player_chell_Armature_standing_jump_portalgun_clip,
};

struct SKAnimationClip* playerDetermineNextClip(struct Player* player, float* blendLerp, float* startTime, struct Vector3* forwardDir, struct Vector3* rightDir) {
    float horzSpeed = player->body.velocity.x * player->body.velocity.x + player->body.velocity.z * player->body.velocity.z;

    int clipOffset = 0;

    if (player->flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun)) {
        clipOffset = 1;
    }

    if (!(player->flags & PlayerFlagsGrounded)) {
        *blendLerp = 0.0f;
        *startTime = 0.0f;
        return gPlayerJumpClips[clipOffset];
    }

    if (horzSpeed < 0.0001f) {
        *blendLerp = 0.0f;
        *startTime = 0.0f;
        return gPlayerIdleClips[clipOffset];
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
            return gPlayerRunSClips[clipOffset];
        } else {
            return gPlayerRunNClips[clipOffset];
        }
    } else {
        if (right > 0.0f) {
            return gPlayerRunEClips[clipOffset];
        } else {
            return gPlayerRunWClips[clipOffset];
        }
    }
}

#define FOOTING_CAST_DISTANCE   (PLAYER_HEAD_HEIGHT + 0.2f)

void playerUpdateFooting(struct Player* player, float maxStandDistance) {
    if (player->anchoredTo){
        player->lastAnchorToPosition = player->anchoredTo->transform.position;
    }
    player->anchoredTo = NULL;

    struct Vector3 castOffset;
    struct Vector3 hitLocation;

    float hitDistance = FOOTING_CAST_DISTANCE;

    vector3Scale(&gUp, &castOffset, -(hitDistance - gPlayerCollider.radius - gPlayerCollider.extendDownward));
    if (collisionObjectCollideShapeCast(&player->collisionObject, &castOffset, &gCollisionScene, &hitLocation)) {
        hitDistance = gPlayerCollider.radius + gPlayerCollider.extendDownward + player->collisionObject.body->transform.position.y - hitLocation.y;
    }

    struct RaycastHit hit;
    struct Ray ray;
    ray.origin = player->body.transform.position;
    vector3Scale(&gUp, &ray.dir, -1.0f);
    if (collisionSceneRaycastOnlyDynamic(&gCollisionScene, &ray, COLLISION_LAYERS_TANGIBLE, hitDistance, &hit)) {
        hitDistance = hit.distance;

        hit.object->flags |= COLLISION_OBJECT_PLAYER_STANDING;

        if (hit.object == player->grabConstraint.object) {
            playerSetGrabbing(player, NULL);
        }

        if (hit.object->body && (hit.object->body->flags & RigidBodyIsKinematic)) {
            player->anchoredTo = hit.object->body;
            player->lastAnchorPoint = hit.at;
            transformPointInverseNoScale(&player->anchoredTo->transform, &hit.at, &player->relativeAnchor);
        }
    }
    
    float penetration = hitDistance - PLAYER_HEAD_HEIGHT;

    if (penetration < 0.0f) {
        vector3AddScaled(&player->body.transform.position, &gUp, MIN(-penetration, maxStandDistance), &player->body.transform.position);
        if (player->body.velocity.y < 0.0f) {
            playerHandleLandingRumble(-player->body.velocity.y);
            player->body.velocity.y = 0.0f;
        }

        if (!(player->flags & PlayerFlagsGrounded)){
            player->flags |= PlayerJustLanded;
        }
        player->flags |= PlayerFlagsGrounded;
    } else {
        player->flags &= ~PlayerFlagsGrounded;
    }
}

void playerPortalFunnel(struct Player* player) {
    if (gCollisionScene.portalTransforms[0] != NULL && gCollisionScene.portalTransforms[1] != NULL){
        struct Transform portal0transform = *gCollisionScene.portalTransforms[0];
        struct Transform portal1transform = *gCollisionScene.portalTransforms[1];
        struct Transform targetPortalTransform;

        //remove z from distance calc
        portal0transform.position.y = player->body.transform.position.y;
        portal1transform.position.y = player->body.transform.position.y;
        
        float portal0dist = vector3DistSqrd(&player->body.transform.position, &portal0transform.position);
        float portal1dist = vector3DistSqrd(&player->body.transform.position, &portal1transform.position);
        float targetDist;

        if (portal0dist < portal1dist){
            targetPortalTransform = portal0transform;
            targetDist = portal0dist;
        } else{
            targetPortalTransform = portal1transform;
            targetDist = portal1dist;
        }

        struct Vector3 straightForward;
        vector3Negate(&gForward, &straightForward);
        quatMultVector(&targetPortalTransform.rotation, &straightForward, &straightForward);
        if (fabsf(straightForward.y) > 0.999f) {
            if (!(player->flags & PlayerFlagsGrounded) && 
                targetDist < (FUNNEL_MAX_DIST*FUNNEL_MAX_DIST) && 
                player->body.velocity.y < FUNNEL_MIN_DOWN_VEL && 
                fabsf(player->body.velocity.x) < FUNNEL_MAX_HORZ_VEL && 
                fabsf(player->body.velocity.z) < FUNNEL_MAX_HORZ_VEL){
                    if (player->body.transform.position.x < targetPortalTransform.position.x - FUNNEL_ACCEPTABLE_CENTERED_DISTANCE){
                        player->body.velocity.x += (FUNNEL_DAMPENING_CONSTANT * (targetPortalTransform.position.x - player->body.transform.position.x));
                    }
                    else if (player->body.transform.position.x > targetPortalTransform.position.x + FUNNEL_ACCEPTABLE_CENTERED_DISTANCE){
                        player->body.velocity.x -= (FUNNEL_DAMPENING_CONSTANT * (player->body.transform.position.x - targetPortalTransform.position.x));
                    }
                    else{
                        player->body.velocity.x *= FUNNEL_CENTERING_CONSTANT;
                    }

                    if (player->body.transform.position.z < targetPortalTransform.position.z - FUNNEL_ACCEPTABLE_CENTERED_DISTANCE){
                        player->body.velocity.z += (FUNNEL_DAMPENING_CONSTANT * (targetPortalTransform.position.z - player->body.transform.position.z));
                    }
                    else if (player->body.transform.position.z > targetPortalTransform.position.z + FUNNEL_ACCEPTABLE_CENTERED_DISTANCE){
                        player->body.velocity.z -= (FUNNEL_DAMPENING_CONSTANT * (player->body.transform.position.z - targetPortalTransform.position.z));
                    }
                    else{
                        player->body.velocity.z *= FUNNEL_CENTERING_CONSTANT;
                    }
            }
        }
    }
}

void playerUpdate(struct Player* player) {
    struct Vector3 forward;
    struct Vector3 right;

    player->passedThroughPortal = 0;

    if (player->flags & PlayerInCutscene) {
        return;
    }

    skBlenderUpdate(&player->animator, player->armature.pose, FIXED_DELTA_TIME);

    int doorwayMask = worldCheckDoorwaySides(&gCurrentLevel->world, &player->lookTransform.position, player->body.currentRoom);
    playerGetMoveBasis(&player->lookTransform, &forward, &right);

    int isDead = playerIsDead(player);

    if (!isDead && (player->flags & PlayerFlagsGrounded) && controllerActionGet(ControllerActionJump)) {
        player->body.velocity.y += JUMP_IMPULSE;
        if (!vector3IsZero(&player->lastAnchorToVelocity) && player->anchoredTo != NULL){
            player->body.velocity.x += player->lastAnchorToVelocity.x;
            player->body.velocity.z += player->lastAnchorToVelocity.z;
        }
        player->flags |= PlayerJustJumped;
        hudResolvePrompt(&gScene.hud, CutscenePromptTypeJump);
    }

    struct Vector3 targetVelocity = gZeroVec;

    float camera_y_modifier = 0.0;
    if (player->flags & PlayerCrouched){
        camera_y_modifier = -0.25;
    }
    else{
        camera_y_modifier = 0.0;
    }

    struct Vector2 moveInput = controllerDirectionGet(ControllerActionMove);
    struct Vector2 lookInput = controllerDirectionGet(ControllerActionRotate);

    if (gSaveData.controls.flags & ControlSaveTankControls) {
        float tmp;
        tmp = moveInput.y;
        moveInput.y = lookInput.y;
        lookInput.y = tmp;
    }

    if (moveInput.x != 0.0f || moveInput.y != 0.0f) {
        hudResolvePrompt(&gScene.hud, CutscenePromptTypeMove);
    }

    if (!isDead) {
        if (vector2MagSqr(&moveInput) > 1.0f){
            vector2Normalize(&moveInput, &moveInput);
        }

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
            struct Vector3 forwardNegate;
            vector3Negate(&forward, &forwardNegate);
            quatLook(&forwardNegate, &gUp, &player->lookTransform.rotation);
        }
        //look behind
        if (controllerActionGet(ControllerActionLookBackward)){
            quatLook(&forward, &gUp, &player->lookTransform.rotation);
        }
    }

    targetVelocity.y = player->body.velocity.y;
    if (!vector3IsZero(&player->lastAnchorToVelocity) && !(player->flags & PlayerFlagsGrounded) && !(player->anchoredTo)){
        targetVelocity.x += player->lastAnchorToVelocity.x/FIXED_DELTA_TIME;
        targetVelocity.z += player->lastAnchorToVelocity.z/FIXED_DELTA_TIME;
    }

    float velocityDot = vector3Dot(&player->body.velocity, &targetVelocity);
    int isAccelerating = velocityDot > 0.0f;
    float acceleration = 0.0f;
    float velocitySqrd = vector3MagSqrd(&player->body.velocity);
    int isFast = velocitySqrd >= PLAYER_SPEED * PLAYER_SPEED;
    playerUpdateSpeedSound(player);

    if (!(player->flags & PlayerFlagsGrounded)) {
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

    if (!vector3IsZero(&player->lastAnchorToPosition) && player->anchoredTo){
        vector3Sub(&player->anchoredTo->transform.position, &player->lastAnchorToPosition, &player->lastAnchorToVelocity);
    }
    else if (!(player->anchoredTo) && (player->flags & PlayerFlagsGrounded)){
        player->lastAnchorToVelocity = gZeroVec;
    }

    struct Box3D sweptBB = player->collisionObject.boundingBox;
    collisionObjectUpdateBB(&player->collisionObject);
    box3DUnion(&sweptBB, &player->collisionObject.boundingBox, &sweptBB);
    collisionObjectCollideMixed(&player->collisionObject, &prevPos, &sweptBB, &gCollisionScene, &gContactSolver);

    playerUpdateFooting(player, STAND_SPEED * FIXED_DELTA_TIME);
    
    struct ContactManifold* manifold = contactSolverNextManifold(&gContactSolver, &player->collisionObject, NULL);

    while (manifold) {
        contactSolverCleanupManifold(manifold);
        manifold = contactSolverNextManifold(&gContactSolver, &player->collisionObject, manifold);
    }

    collisionObjectUpdateBB(&player->collisionObject);
    playerHandleCollision(player);

    player->body.transform.rotation = player->lookTransform.rotation;    

    int didPassThroughPortal = rigidBodyCheckPortals(&player->body);
    player->passedThroughPortal = didPassThroughPortal;

    player->lookTransform.position = player->body.transform.position;
    player->lookTransform.position.y += camera_y_modifier;

    //if player is shaking, shake screen 
    playerShakeUpdate(player);

    player->lookTransform.rotation = player->body.transform.rotation;
    quatIdent(&player->body.transform.rotation);

    if (didPassThroughPortal) {
        soundPlayerPlay(soundsPortalEnter[didPassThroughPortal - 1], 0.75f, 1.0f, NULL, NULL, SoundTypeAll);
        hudShowSubtitle(&gScene.hud, PORTALPLAYER_ENTERPORTAL, SubtitleTypeCaption);
        soundPlayerPlay(soundsPortalExit[2 - didPassThroughPortal], 0.75f, 1.0f, NULL, NULL, SoundTypeAll);
        hudShowSubtitle(&gScene.hud, PORTALPLAYER_EXITPORTAL, SubtitleTypeCaption);
        gPlayerCollider.extendDownward = 0.0f;
    } else {
        gPlayerCollider.extendDownward = mathfMoveTowards(gPlayerCollider.extendDownward, TARGET_CAPSULE_EXTEND_HEIGHT, STAND_SPEED * FIXED_DELTA_TIME);
    }

    float rotateRate = mathfLerp(MIN_ROTATE_RATE, MAX_ROTATE_RATE, (float)gSaveData.controls.sensitivity / 0xFFFF);
    float targetYaw = -lookInput.x * rotateRate;
    float targetPitch = lookInput.y * rotateRate;

    if (gSaveData.controls.flags & ControlSaveFlagsInvert) {
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

    collisionObjectUpdateBB(&player->collisionObject);

    if (!didPassThroughPortal) {
        player->body.currentRoom = worldCheckDoorwayCrossings(&gCurrentLevel->world, &player->lookTransform.position, player->body.currentRoom, doorwayMask);
    }
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


    if ((gSaveData.controls.flags & ControlSavePortalFunneling) && (vector2MagSqr(&moveInput) == 0.0f)){
        playerPortalFunnel(player);
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

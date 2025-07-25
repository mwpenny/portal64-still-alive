#include "player.h"

#include "player/grab_rotation.h"
#include "player_rumble_clips.h"

#include "audio/clips.h"
#include "audio/soundplayer.h"
#include "defs.h"
#include "controls/controller_actions.h"
#include "levels/levels.h"
#include "math/mathf.h"
#include "physics/collision_capsule.h"
#include "physics/collision_scene.h"
#include "physics/collision.h"
#include "physics/config.h"
#include "physics/contact_insertion.h"
#include "physics/point_constraint.h"
#include "savefile/savefile.h"
#include "scene/ball.h"
#include "system/time.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/player/chell.h"
#include "codegen/assets/models/portal_gun/w_portalgun.h"

#define GRAB_RAYCAST_DISTANCE   2.5f
#define GRAB_MIN_OFFSET_Y      -1.1f
#define GRAB_MAX_OFFSET_Y       1.25f

#define STEP_TIME               0.35f

#define STAND_SPEED             1.5f
#define SHAKE_DISTANCE          0.02f

#define ENV_DAMAGE_OVERLAY_TIME 0.5f
#define ENV_DAMAGE_OVERLAY_FADE 0.75f
#define HEALTH_REGEN_DELAY      1.0f
#define HEALTH_REGEN_SPEED      60.0f
#define DEAD_OFFSET             -0.4f

#define PLAYER_COLLISION_LAYERS (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_BALL | COLLISION_LAYERS_BLOCK_TURRET_SHOTS)

#define FUNNEL_STRENGTH         5.0f
#define FUNNEL_CENTER_TOLERANCE 0.1f
#define FUNNEL_CENTER_SLOWDOWN  0.05f
#define FUNNEL_STOP_DIST        0.6f
#define FUNNEL_STOP_HEIGHT      0.25f
#define FUNNEL_MAX_HORIZ_DIST   1.2f
#define FUNNEL_MIN_DOWN_VEL     -2.25f
#define FUNNEL_MAX_HORIZ_VEL    7.0f

#define PLAYER_SPEED            (150.0f / 64.0f)
#define PLAYER_AIR_SPEED        (60.0f  / 64.0f)
#define PLAYER_ACCEL            (20.0f)
#define PLAYER_AIR_ACCEL        (15.0f)
#define PLAYER_FRICTION         (5.875f)

#define FRICTION_STOP_THRESHOLD (190.0f / 64.0f)
#define FLING_THRESHOLD_VEL     (5.0f)
#define JUMP_BOOST_LIMIT        (PLAYER_SPEED * 1.5f)

#define MIN_ROTATE_RATE         (M_PI * 0.5f)
#define MAX_ROTATE_RATE         (M_PI * 3.5f)

#define MIN_ROTATE_RATE_DELTA   (M_PI * 0.06125f)
#define MAX_ROTATE_RATE_DELTA   MAX_ROTATE_RATE

#define JUMP_IMPULSE            2.5f
#define THROW_IMPULSE           1.25f

#define PLAYER_GRABBING_THROUGH_NOTHING 0

struct Vector3 gGrabDistance = {0.0f, 0.0f, -1.5f};
struct Vector3 gCameraOffset = {0.0f, 0.0f, 0.0f};

struct CollisionQuad gPlayerColliderFaces[8];

#define TARGET_CAPSULE_EXTEND_HEIGHT   0.45f

struct CollisionCapsule gPlayerCollider = {
    0.25f,
    TARGET_CAPSULE_EXTEND_HEIGHT,
};

struct ColliderTypeData gPlayerColliderData = {
    CollisionShapeTypeCapsule,
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

    playerGetMoveBasis(&player->lookTransform.rotation, &forwardVector, &unusedRight);
    
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

static void playerHandleCollideStartEnd(struct CollisionObject* object, struct CollisionObject* other, struct Vector3* normal) {
    if (normal == NULL) {
        return;
    }

    float normalVelocity = vector3Dot(normal, &object->body->velocity);
    if (normalVelocity < 0.0f) {
        playerHandleLandingRumble(-normalVelocity);
    }
}

void playerInit(struct Player* player, struct Location* startLocation, struct Vector3* velocity) {
    player->flyingSoundLoopId = soundPlayerPlay(soundsFastFalling, 0.0f, 0.5f, NULL, NULL, SoundTypeAll);

    collisionObjectInit(&player->collisionObject, &gPlayerColliderData, &player->body, 1.0f, PLAYER_COLLISION_LAYERS);
    player->collisionObject.collideStartEnd = playerHandleCollideStartEnd;

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
    player->flags = PlayerFlagsGrounded;
    player->health = PLAYER_MAX_HEALTH;
    player->healthRegenTimer = HEALTH_REGEN_DELAY;
    player->stepTimer = STEP_TIME;
    player->shakeTimer = 0.0f;
    player->currentFoot = 0;
    player->passedThroughPortal = 0;
    player->jumpImpulse = JUMP_IMPULSE;

    // player->flags |= PlayerHasFirstPortalGun | PlayerHasSecondPortalGun;

    player->dynamicId = dynamicSceneAdd(player, playerRender, &player->body.transform.position, 1.5f);
    dynamicSceneSetFlags(player->dynamicId, DYNAMIC_SCENE_OBJECT_SKIP_ROOT);

    playerSetLocation(player, startLocation);
}

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
            playerDamage(player, PLAYER_MAX_HEALTH, PlayerDamageTypeEnemy);
            soundPlayerPlay(soundsBallKill, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
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
    } else if (grabbing && !player->grabConstraint.object) {
        pointConstraintInit(&player->grabConstraint, grabbing, 8.0f, 5.0f, 1.0f);
        contactSolverAddPointConstraint(&gContactSolver, &player->grabConstraint);
        hudResolvePrompt(&gScene.hud, CutscenePromptTypePickup);
        portalGunPickup(&gScene.portalGun);
    } else if (!grabbing && player->grabConstraint.object) {
        player->grabConstraint.object = NULL;
        contactSolverRemovePointConstraint(&gContactSolver, &player->grabConstraint);
        hudResolvePrompt(&gScene.hud, CutscenePromptTypeDrop);
        player->grabbingThroughPortal = PLAYER_GRABBING_THROUGH_NOTHING;
        portalGunRelease(&gScene.portalGun);
    } else if (grabbing != player->grabConstraint.object) {
        pointConstraintInit(&player->grabConstraint, grabbing, 8.0f, 5.0f, 1.0f);
    }
    
    playerInitGrabRotationBase(player);
}

void playerInitGrabRotationBase(struct Player* player) {
    if (!player->grabConstraint.object) {
        return;
    }
    struct Quaternion forwardRotation = player->lookTransform.rotation;
    struct Vector3 forward, tmpVec;
    playerGetMoveBasis(&forwardRotation, &forward, &tmpVec);
    vector3Negate(&forward, &forward);
    quatLook(&forward, &gUp, &forwardRotation);
    playerPortalGrabTransform(player, NULL, &forwardRotation);
    
    enum GrabRotationFlags grabRotationFlags = grabRotationFlagsForCollisionObject(player->grabConstraint.object);
    grabRotationInitBase(grabRotationFlags, &forwardRotation, &player->grabConstraint.object->body->transform.rotation, &player->grabRotationBase);
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

int playerIsGrabbingObject(struct Player* player, struct CollisionObject* object) {
    return player->grabConstraint.object == object;
}

void playerThrowObject(struct Player* player) {
    if (!playerIsGrabbing(player)) {
        return;
    }
    struct Quaternion throwRotation = player->lookTransform.rotation;
    playerPortalGrabTransform(player, NULL, &throwRotation);

    struct Vector3 forward, right;
    playerGetMoveBasis(&throwRotation, &forward, &right);
    
    struct CollisionObject* object = player->grabConstraint.object;
    playerSetGrabbing(player, NULL);
    
    // scale impulse with mass to throw each object the same distance
    vector3Scale(&forward, &forward, -1.0f * THROW_IMPULSE * object->body->mass);
    rigidBodyApplyImpulse(object->body, &object->body->transform.position, &forward);
}

int playerRaycastGrab(struct Player* player, struct RaycastHit* hit, int checkPastObject) {
    struct Ray ray;

    ray.origin = player->lookTransform.position;
    quatMultVector(&player->lookTransform.rotation, &gForward, &ray.dir);
    vector3Negate(&ray.dir, &ray.dir);
    int result;

    short prevCollisionLayers = player->collisionObject.collisionLayers;
    player->collisionObject.collisionLayers = 0;

    if (checkPastObject){
        short prevObjectCollisionLayers = player->grabConstraint.object->collisionLayers;
        player->grabConstraint.object->collisionLayers = 0;
        result = collisionSceneRaycast(&gCollisionScene, player->body.currentRoom, &ray, COLLISION_LAYERS_TANGIBLE, GRAB_RAYCAST_DISTANCE, 1, hit);
        player->grabConstraint.object->collisionLayers = prevObjectCollisionLayers;
    }
    else{
        result = collisionSceneRaycast(&gCollisionScene, player->body.currentRoom, &ray, COLLISION_LAYERS_GRABBABLE | COLLISION_LAYERS_TANGIBLE, GRAB_RAYCAST_DISTANCE, 1, hit);
    }

    player->collisionObject.collisionLayers = prevCollisionLayers;

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

                // Raycasts only check portals if something else was hit first
                // (i.e., a wall), so grab casts also check the TANGIBLE layer.
                //
                // Make sure we actually found something to interact with.
                if (hit.object->body && (hit.object->collisionLayers & COLLISION_LAYERS_GRABBABLE)) {
                    player->flags |= PlayerJustSelect;

                    if ((hit.object->body->flags & RigidBodyFlagsGrabbable) &&
                        !(hit.object->flags & COLLISION_OBJECT_PLAYER_STANDING)) {
                        // Grabbable object
                        player->grabbingThroughPortal = hit.numPortalsPassed;
                        playerSetGrabbing(player, hit.object);
                    } else if (!(hit.object->body->flags & RigidBodyFlagsGrabbable)) {
                        // Usable object
                        hudResolvePrompt(&gScene.hud, CutscenePromptTypeUse);
                    }
                } else {
                    player->flags |= PlayerJustDeniedSelect;
                }
            } else {
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
        vector3Multiply(&player->lookTransform.scale, &temp_grab_dist, &temp_grab_dist);
        
        struct Vector3 grabPoint;
        
        // determine object target height
        quatMultVector(&player->lookTransform.rotation, &temp_grab_dist, &grabPoint);
        float grabY = maxf(minf(grabPoint.y, GRAB_MAX_OFFSET_Y), GRAB_MIN_OFFSET_Y);
        
        // keep object at steady XZ-planar distance in front of player
        struct Quaternion forwardRotation;
        struct Vector3 forward, forwardNegate, right;
        playerGetMoveBasis(&player->lookTransform.rotation, &forward, &right);
        vector3Negate(&forward, &forwardNegate);
        quatLook(&forwardNegate, &gUp, &forwardRotation);
        quatMultVector(&forwardRotation, &temp_grab_dist, &grabPoint);
        vector3Add(&player->lookTransform.position, &grabPoint, &grabPoint);
        grabPoint.y += grabY;
        
        // remember delta between forwardRotation and lookTransform.rotation
        struct Quaternion lookRotationDelta, forwardRotationInv;
        quatConjugate(&forwardRotation, &forwardRotationInv);
        quatMultiply(&forwardRotationInv, &player->lookTransform.rotation, &lookRotationDelta);
        
        if (player->grabbingThroughPortal != PLAYER_GRABBING_THROUGH_NOTHING) {
            if (!collisionSceneIsPortalOpen()) {
                // portal was closed while holding object through it
                playerSetGrabbing(player, NULL);
                return;
            }

            playerPortalGrabTransform(player, &grabPoint, &forwardRotation);
        }
        
        struct Quaternion grabRotation;
        enum GrabRotationFlags grabRotationFlags = grabRotationFlagsForCollisionObject(player->grabConstraint.object);
        grabRotationUpdate(grabRotationFlags, &lookRotationDelta, &forwardRotation, &player->grabRotationBase, &grabRotation);
        
        pointConstraintUpdateTarget(&player->grabConstraint, &grabPoint, &grabRotation);
    }
}

void playerGetMoveBasis(struct Quaternion* rotation, struct Vector3* forward, struct Vector3* right) {
    quatMultVector(rotation, &gForward, forward);
    quatMultVector(rotation, &gRight, right);

    if (forward->y > 0.7f) {
        quatMultVector(rotation, &gUp, forward);
        vector3Negate(forward, forward);
    } else if (forward->y < -0.7f) {
        quatMultVector(rotation, &gUp, forward);
    }

    forward->y = 0.0f;
    right->y = 0.0f;

    vector3Normalize(forward, forward);
    vector3Normalize(right, right);
}

void playerPortalGrabTransform(struct Player* player, struct Vector3* grabPoint, struct Quaternion* grabRotation) {
    if (player->grabbingThroughPortal == PLAYER_GRABBING_THROUGH_NOTHING) {
        return;
    }
    struct Transform pointTransform;
    collisionSceneGetPortalTransform(player->grabbingThroughPortal > 0 ? 0 : 1, &pointTransform);
    
    for (int i = 0; i < abs(player->grabbingThroughPortal); ++i) {
        if (grabPoint) {
            transformPoint(&pointTransform, grabPoint, grabPoint);
        }
        struct Quaternion finalRotation;
        quatMultiply(&pointTransform.rotation, grabRotation, &finalRotation);
        *grabRotation = finalRotation;
    }
}

void playerGivePortalGun(struct Player* player, int flags) {
    player->flags |= flags;
}

void playerSetLocation(struct Player* player, struct Location* location) {
    if (location) {
        player->lookTransform = location->transform;
        player->body.currentRoom = location->roomIndex;
    } else {
        transformInitIdentity(&player->lookTransform);
        player->body.currentRoom = 0;
    }
    player->body.transform = player->lookTransform;

    player->anchoredTo = NULL;
    player->anchorLastPosition = gZeroVec;

    collisionObjectUpdateBB(&player->collisionObject);
    dynamicSceneSetRoomFlags(player->dynamicId, ROOM_FLAG_FROM_INDEX(player->body.currentRoom));
}

void playerUpdateSounds(struct Player* player) {
    enum PlayerFlags flags = player->flags;

    if ((flags & PlayerFlagsGrounded) && (flags & PlayerIsStepping)) {
        soundPlayerPlay(soundsConcreteFootstep[player->currentFoot], 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
        player->flags &= ~PlayerIsStepping;
    }
    if (flags & PlayerJustJumped) {
        soundPlayerPlay(soundsConcreteFootstep[3], 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
        player->flags &= ~PlayerJustJumped;
    }
    if (flags & PlayerJustLanded) {
        // TODO: Dead body sound when landing on ground while dead
        soundPlayerPlay(soundsConcreteFootstep[2], 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
        player->flags &= ~PlayerJustLanded;
    }
    if (flags & PlayerJustSelect) {
        soundPlayerPlay(soundsSelecting[1], 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
        player->flags &= ~PlayerJustSelect;
    }
    if (flags & PlayerJustDeniedSelect) {
        if (flags & PlayerHasFirstPortalGun) {
            soundPlayerPlay(soundsSelecting[0], 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
        } else {
            soundPlayerPlay(soundsSelecting[2], 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
        }
        player->flags &= ~PlayerJustDeniedSelect;
    }
    if (player->passedThroughPortal) {
        soundPlayerPlay(soundsPortalEnter[player->passedThroughPortal - 1], 0.75f, 0.5f, NULL, NULL, SoundTypeAll);
        hudShowSubtitle(&gScene.hud, PORTALPLAYER_ENTERPORTAL, SubtitleTypeCaption);
        soundPlayerPlay(soundsPortalExit[2 - player->passedThroughPortal], 0.75f, 0.5f, NULL, NULL, SoundTypeAll);
        hudShowSubtitle(&gScene.hud, PORTALPLAYER_EXITPORTAL, SubtitleTypeCaption);
    }

    float flyingSoundVolume = clampf(
        sqrtf(vector3MagSqrd(&player->body.velocity))*(0.6f / MAX_PORTAL_SPEED),
        0.0f, 1.0f
    );
    soundPlayerAdjustVolume(player->flyingSoundLoopId, flyingSoundVolume);
    if (flyingSoundVolume >= 0.75){
        hudShowSubtitle(&gScene.hud, PORTALPLAYER_WOOSH, SubtitleTypeCaption);
    }
}

void playerUpdateHealth(struct Player* player) {
    if (player->health <= 0.0f || player->health >= PLAYER_MAX_HEALTH) {
        return;
    }

    if (player->healthRegenTimer > 0.0f) {
        player->healthRegenTimer -= FIXED_DELTA_TIME;
    }

    if (player->healthRegenTimer <= 0.0f) {
        player->healthRegenTimer = 0.0f;
        player->health = MIN(player->health + (HEALTH_REGEN_SPEED * FIXED_DELTA_TIME), PLAYER_MAX_HEALTH);
    }
}

void playerDamage(struct Player* player, float amount, enum PlayerDamageType damageType) {
    if ((player->flags & PlayerIsInvincible) || player->health <= 0.0f) {
        return;
    }

    if (damageType == PlayerDamageTypeEnvironment) {
        hudShowColoredOverlay(
            &gScene.hud,
            &gColorWhite,
            ENV_DAMAGE_OVERLAY_TIME,
            ENV_DAMAGE_OVERLAY_FADE
        );
    }

    player->health -= amount;
    player->healthRegenTimer = HEALTH_REGEN_DELAY;

    if (player->health <= 0.0f) {
        player->health = 0.0f;

        // Drop the portal gun
        player->flags &= ~(PlayerHasFirstPortalGun | PlayerHasSecondPortalGun);
        playerSetGrabbing(player, NULL);

        rumblePakClipPlay(&gPlayerDieRumbleWave);
    } else {
        rumblePakClipPlay(&gPlayerDamageRumbleWave);
    }
}

int playerIsDead(struct Player* player) {
    return player->health <= 0.0f;
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
    // Clamp hit distance to nearest static collider (if any)
    struct Vector3 castOffset;
    struct Vector3 hitLocation;
    float hitDistance = FOOTING_CAST_DISTANCE;
    vector3Scale(&gUp, &castOffset, -(hitDistance - gPlayerCollider.radius - gPlayerCollider.extendDownward));
    if (collisionObjectCollideShapeCast(&player->collisionObject, &castOffset, &gCollisionScene, &hitLocation)) {
        hitDistance = gPlayerCollider.radius + gPlayerCollider.extendDownward + player->collisionObject.body->transform.position.y - hitLocation.y;
    }

    // Find dynamic object below feet
    struct RaycastHit hit;
    struct Ray ray;
    hit.roomIndex = player->body.currentRoom;
    ray.origin = player->body.transform.position;
    vector3Scale(&gUp, &ray.dir, -1.0f);

    struct RigidBody* anchor = NULL;

    short prevCollisionLayers = player->collisionObject.collisionLayers;
    player->collisionObject.collisionLayers = 0;

    if (collisionSceneRaycastOnlyDynamic(&gCollisionScene, &ray, COLLISION_LAYERS_TANGIBLE, hitDistance, &hit)) {
        hitDistance = hit.distance;

        hit.object->flags |= COLLISION_OBJECT_PLAYER_STANDING;

        if (hit.object == player->grabConstraint.object) {
            playerSetGrabbing(player, NULL);
        }

        if (hit.object->body && (hit.object->body->flags & RigidBodyIsKinematic)) {
            anchor = hit.object->body;
        }
    }

    player->collisionObject.collisionLayers = prevCollisionLayers;
    
    // Stand on collision
    float penetration = hitDistance - PLAYER_HEAD_HEIGHT;
    if (penetration < 0.00001f) {
        vector3AddScaled(&player->body.transform.position, &gUp, MIN(-penetration, maxStandDistance), &player->body.transform.position);
        if (player->body.velocity.y < 0.0f) {
            playerHandleLandingRumble(-player->body.velocity.y);
            player->body.velocity.y = 0.0f;
        }

        if (!(player->flags & PlayerFlagsGrounded)){
            player->flags |= PlayerJustLanded;
        }
        player->flags |= PlayerFlagsGrounded;

        player->anchoredTo = anchor;
    } else {
        player->flags &= ~PlayerFlagsGrounded;

        // Stay anchored so we can follow it in the air
    }

    if (player->anchoredTo) {
        // Save last position so the anchor can be followed
        player->anchorLastPosition = player->anchoredTo->transform.position;
    } else {
        player->anchorLastPosition = gZeroVec;
    }

    // Update state for footstep sounds
    if ((player->flags & PlayerFlagsGrounded) && (player->body.velocity.x == 0) && (player->body.velocity.z == 0)) {
        player->stepTimer = STEP_TIME;
        player->flags &= ~PlayerIsStepping;
    } else {
        player->stepTimer -= FIXED_DELTA_TIME;

        if (player->stepTimer < 0.0f) {
            player->flags |= PlayerIsStepping;
            player->stepTimer = STEP_TIME;
            player->currentFoot = !player->currentFoot;
        }
    }
}

void playerPortalFunnel(struct Player* player, struct Vector3* targetVelocity) {
    if (gCollisionScene.portalTransforms[0] == NULL || gCollisionScene.portalTransforms[1] == NULL) {
        return;
    }

    struct Vector3 portal0HorizPos = gCollisionScene.portalTransforms[0]->position;
    struct Vector3 portal1HorizPos = gCollisionScene.portalTransforms[1]->position;
    struct Transform* targetPortalTransform;

    portal0HorizPos.y = player->body.transform.position.y;
    portal1HorizPos.y = player->body.transform.position.y;

    float portal0HorizDist = vector3DistSqrd(&player->body.transform.position, &portal0HorizPos);
    float portal1HorizDist = vector3DistSqrd(&player->body.transform.position, &portal1HorizPos);
    float targetHorizDist;

    if (portal0HorizDist < portal1HorizDist) {
        targetPortalTransform = gCollisionScene.portalTransforms[0];
        targetHorizDist = portal0HorizDist;
    } else {
        targetPortalTransform = gCollisionScene.portalTransforms[1];
        targetHorizDist = portal1HorizDist;
    }

    struct Vector3 portalNormal;
    vector3Negate(&gForward, &portalNormal);
    quatMultVector(&targetPortalTransform->rotation, &portalNormal, &portalNormal);
    if (fabsf(portalNormal.y) < 0.999f) {
        // Not a floor portal
        return;
    }

    if (player->flags & PlayerFlagsGrounded ||
        targetHorizDist > (FUNNEL_MAX_HORIZ_DIST * FUNNEL_MAX_HORIZ_DIST) ||
        player->body.velocity.y > FUNNEL_MIN_DOWN_VEL ||
        fabsf(player->body.velocity.x) > FUNNEL_MAX_HORIZ_VEL ||
        fabsf(player->body.velocity.z) > FUNNEL_MAX_HORIZ_VEL) {

        return;
    }

    struct Vector3 targetOffset;
    vector3Sub(&targetPortalTransform->position, &player->body.transform.position, &targetOffset);

    if (targetHorizDist < (FUNNEL_STOP_DIST * FUNNEL_STOP_DIST) && targetOffset.y > -FUNNEL_STOP_HEIGHT) {
        // Don't carry horizontal momentum through fling (helps land where intended)
        player->body.velocity.x = 0.0f;
        player->body.velocity.z = 0.0f;
    } else {
        if (fabsf(targetOffset.x) > FUNNEL_CENTER_TOLERANCE) {
            targetVelocity->x += FUNNEL_STRENGTH * targetOffset.x;
        } else {
            player->body.velocity.x *= FUNNEL_CENTER_SLOWDOWN;
        }

        if (fabsf(targetOffset.z) > FUNNEL_CENTER_TOLERANCE) {
            targetVelocity->z += FUNNEL_STRENGTH * targetOffset.z;
        } else {
            player->body.velocity.z *= FUNNEL_CENTER_SLOWDOWN;
        }
    }
}

void playerProcessInput(struct Player* player, struct Vector3* forward, struct Vector3* right, struct Vector2* moveInput, struct Vector2* lookInput) {
    *moveInput = controllerDirectionGet(ControllerActionMove);
    *lookInput = controllerDirectionGet(ControllerActionRotate);

    if (gSaveData.controls.flags & ControlSaveTankControls) {
        float tmp;
        tmp = moveInput->y;
        moveInput->y = lookInput->y;
        lookInput->y = tmp;
    }

    vector2Normalize(moveInput, moveInput);

    if (!playerIsDead(player)) {
        if (controllerActionGet(ControllerActionDuck)) {
            player->flags ^= PlayerCrouched;
        }

        if (controllerActionGet(ControllerActionLookForward)) {
            struct Vector3 forwardNegate;
            vector3Negate(forward, &forwardNegate);
            quatLook(&forwardNegate, &gUp, &player->lookTransform.rotation);
        }

        if (controllerActionGet(ControllerActionLookBackward)) {
            quatLook(forward, &gUp, &player->lookTransform.rotation);
        }
    }
}

void playerJump(struct Player* player, struct Vector3* targetVelocity, struct Vector3* forward) {
    vector3AddScaled(
        &player->body.velocity,
        forward,
        vector3Dot(targetVelocity, forward) * 0.5f,
        &player->body.velocity
    );

    float speed = vector3MagSqrd(&player->body.velocity);
    if (speed > JUMP_BOOST_LIMIT * JUMP_BOOST_LIMIT) {
        vector3Scale(&player->body.velocity, &player->body.velocity, JUMP_BOOST_LIMIT / sqrtf(speed));
    }

    player->body.velocity.y += player->jumpImpulse;

    player->flags &= ~PlayerFlagsGrounded;
    player->flags |= PlayerJustJumped;

    hudResolvePrompt(&gScene.hud, CutscenePromptTypeJump);
}

void playerApplyFriction(struct Player* player) {
    if (vector3IsZero(&player->body.velocity)) {
        return;
    }

    float currentSpeed = sqrtf(vector3MagSqrd(&player->body.velocity));
    if (currentSpeed < 0.01f) {
        player->body.velocity = gZeroVec;
        return;
    }

    float stopAccel;
    if (currentSpeed < FRICTION_STOP_THRESHOLD) {
        stopAccel = FRICTION_STOP_THRESHOLD;
    } else if (currentSpeed < (FRICTION_STOP_THRESHOLD * 2.0f)) {
        stopAccel = currentSpeed;
    } else {
        stopAccel = currentSpeed * 0.5625f;
    }

    float targetSpeed = MAX(
        currentSpeed - (stopAccel * PLAYER_FRICTION * FIXED_DELTA_TIME),
        0.0f
    );

    vector3Scale(
        &player->body.velocity,
        &player->body.velocity,
        targetSpeed / currentSpeed
    );
}

void playerAccelerate(struct Player* player, struct Vector3* targetVelocity, float acceleration, float maxSpeed) {
    if (!vector3IsZero(targetVelocity)) {
        struct Vector3 targetDirection;
        vector3Normalize(targetVelocity, &targetDirection);

        float targetSpeed = MIN(
            sqrtf(vector3MagSqrd(targetVelocity)),
            maxSpeed
        );

        // Accelerate more as the player diverges from the movement direction
        // Limiting this projection, not velocity, is what enables bunny hopping / air strafing
        float velocityProj = vector3Dot(&player->body.velocity, &targetDirection);
        float maxAcceleration = targetSpeed - velocityProj;

        if (maxAcceleration > 0.0f) {
            vector3AddScaled(
                &player->body.velocity,
                &targetDirection,
                MIN(acceleration * targetSpeed * FIXED_DELTA_TIME, maxAcceleration),
                &player->body.velocity
            );
        }
    }

    player->body.angularVelocity = gZeroVec;
    player->body.velocity.y += GRAVITY_CONSTANT * FIXED_DELTA_TIME;
}

void playerMove(struct Player* player, struct Vector2* moveInput, struct Vector3* forward, struct Vector3* right) {
    int hasMoveInput = !vector2IsZero(moveInput);
    struct Vector3 targetVelocity = gZeroVec;

    if (hasMoveInput) {
        vector3AddScaled(&targetVelocity, right, PLAYER_SPEED * moveInput->x, &targetVelocity);
        vector3AddScaled(&targetVelocity, forward, -PLAYER_SPEED * moveInput->y, &targetVelocity);

        hudResolvePrompt(&gScene.hud, CutscenePromptTypeMove);
    }

    // Do this first so skilled players can avoid friction on the first frame
    if ((player->flags & PlayerFlagsGrounded) && controllerActionGet(ControllerActionJump)) {
        playerJump(player, &targetVelocity, forward);
    }

    if (player->flags & PlayerFlagsGrounded) {
        playerApplyFriction(player);
        playerAccelerate(player, &targetVelocity, PLAYER_ACCEL, PLAYER_SPEED);
    } else {
        // Prevent moving against flings. Axis-aligned only, to prevent breaking out with side movement.
        if (fabsf(player->body.velocity.x) > FLING_THRESHOLD_VEL && (targetVelocity.x * player->body.velocity.x) < 0.0f) {
            targetVelocity.x = 0.0f;
        }
        if (fabsf(player->body.velocity.z) > FLING_THRESHOLD_VEL && (targetVelocity.z * player->body.velocity.z) < 0.0f) {
            targetVelocity.z = 0.0f;
        }

        if ((gSaveData.controls.flags & ControlSavePortalFunneling) && !hasMoveInput) {
            playerPortalFunnel(player, &targetVelocity);
        }

        playerAccelerate(player, &targetVelocity, PLAYER_AIR_ACCEL, PLAYER_AIR_SPEED);
    }

    if (playerIsDead(player)) {
        player->body.velocity.x = 0.0f;
        player->body.velocity.z = 0.0f;
    } else if (player->anchoredTo) {
        // Follow moving platform
        struct Vector3 anchorOffset;
        vector3Sub(&player->anchoredTo->transform.position, &player->anchorLastPosition, &anchorOffset);
        vector3Add(&player->body.transform.position, &anchorOffset, &player->body.transform.position);
    }

    vector3AddScaled(&player->body.transform.position, &player->body.velocity, FIXED_DELTA_TIME, &player->body.transform.position);
}

void playerUpdateCamera(struct Player* player, struct Vector2* lookInput, int didPassThroughPortal) {
    float camera_y_modifier = (player->flags & PlayerCrouched) ? -0.25f : 0.0f;

    player->lookTransform.position = player->body.transform.position;
    player->lookTransform.position.y += camera_y_modifier;
    player->lookTransform.rotation = player->body.transform.rotation;

    // If player is shaking, shake screen
    playerShakeUpdate(player);

    // Compute yaw and pitch velocities
    float rotateRate = mathfLerp(MIN_ROTATE_RATE, MAX_ROTATE_RATE, (float)gSaveData.controls.sensitivity / 0xFFFF);
    float targetYaw = -lookInput->x * rotateRate;
    float targetPitch = lookInput->y * rotateRate;
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

    // Move roll toward correct orientation (i.e., after going through a portal upside down)
    // If player is looking close to directly up or down, there is no need and doing so will:
    //   a) Create jitter (if standing)
    //   b) Flip the view undesirably (going through a portal vertically while looking into it)
    if (fabsf(lookingForward.y) < 0.99f) {
        struct Quaternion upRotation;
        quatLook(&lookingForward, &gUp, &upRotation);
        quatLerp(&upRotation, &player->lookTransform.rotation, 0.9f, &player->lookTransform.rotation);
    }

    // Apply yaw velocity
    struct Quaternion deltaRotate;
    quatAxisAngle(&gUp, player->yawVelocity * FIXED_DELTA_TIME, &deltaRotate);
    struct Quaternion postYawRotate;
    quatMultiply(&deltaRotate, &player->lookTransform.rotation, &postYawRotate);

    // Apply pitch velocity
    quatAxisAngle(&gRight, player->pitchVelocity * FIXED_DELTA_TIME, &deltaRotate);
    quatMultiply(&postYawRotate, &deltaRotate, &player->lookTransform.rotation);

    // Prevent player from looking too far up or down
    vector3Negate(&gForward, &lookingForward);
    quatMultVector(&postYawRotate, &lookingForward, &lookingForward);
    struct Vector3 newLookingForward;
    vector3Negate(&gForward, &newLookingForward);
    quatMultVector(&player->lookTransform.rotation, &newLookingForward, &newLookingForward);

    float pitchSign = signf(player->pitchVelocity);

    if (!didPassThroughPortal && lookingForward.y * pitchSign > newLookingForward.y * pitchSign && lookingForward.y * pitchSign > 0.0f) {
        // Snap to up/down
        struct Vector3 newForward = gZeroVec;
        newForward.y = pitchSign;
        struct Vector3 newUp;
        quatMultVector(&postYawRotate, &gUp, &newUp);
        quatLook(&newForward, &newUp, &player->lookTransform.rotation);
        player->pitchVelocity = 0.0f;
    }
}

void playerUpdate(struct Player* player) {
    if (player->flags & PlayerInCutscene) {
        return;
    }

    skBlenderUpdate(&player->animator, player->armature.pose, FIXED_DELTA_TIME);

    struct Vector3 forward;
    struct Vector3 right;
    playerGetMoveBasis(&player->lookTransform.rotation, &forward, &right);

    struct Vector2 moveInput;
    struct Vector2 lookInput;
    playerProcessInput(player, &forward, &right, &moveInput, &lookInput);

    struct Vector3 prevPos = player->body.transform.position;
    int doorwayMask = worldCheckDoorwaySides(&gCurrentLevel->world, &player->lookTransform.position, player->body.currentRoom);

    playerMove(player, &moveInput, &forward, &right);

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
    if (didPassThroughPortal) {
        gPlayerCollider.extendDownward = 0.0f;
    } else {
        gPlayerCollider.extendDownward = mathfMoveTowards(gPlayerCollider.extendDownward, TARGET_CAPSULE_EXTEND_HEIGHT, STAND_SPEED * FIXED_DELTA_TIME);
    }

    playerUpdateCamera(player, &lookInput, didPassThroughPortal);
    quatIdent(&player->body.transform.rotation);

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
    
    playerUpdateHealth(player);
    playerUpdateSounds(player);
}

void playerApplyCameraTransform(struct Player* player, struct Transform* cameraTransform) {
    cameraTransform->rotation = player->lookTransform.rotation;
    cameraTransform->position = player->lookTransform.position;

    if (playerIsDead(player)) {
        cameraTransform->position.y += DEAD_OFFSET;
    }
}

void playerToggleJumpImpulse(struct Player* player, float newJumpImpulse) {
    if (player->jumpImpulse == JUMP_IMPULSE) {
        player->jumpImpulse = newJumpImpulse;
    } else {
        player->jumpImpulse = JUMP_IMPULSE;
    }
}

void playerToggleInvincibility(struct Player* player) {
    player->flags ^= PlayerIsInvincible;
}

void playerToggleCollisionLayers(struct Player* player, short collisionLayers) {
    player->collisionObject.collisionLayers ^= collisionLayers;
}
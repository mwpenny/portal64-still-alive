#include "scene_serialize.h"

#include <assert.h>

#include "decor/decor_object_list.h"
#include "levels/levels.h"
#include "physics/collision_scene.h"
#include "util/memory.h"

struct PartialTransform {
    struct Vector3 position;
    struct Quaternion rotation;
};

static void playerSerialize(struct Serializer* serializer, SerializeAction action, struct Player* player) {
    action(serializer, &player->lookTransform, sizeof(struct PartialTransform));
    action(serializer, &player->body.velocity, sizeof(player->body.velocity));
    action(serializer, &player->body.currentRoom, sizeof(player->body.currentRoom));
    action(serializer, &player->flags, sizeof(player->flags));
    action(serializer, &player->health, sizeof(player->health));
    action(serializer, &player->grabbingThroughPortal, sizeof(player->grabbingThroughPortal));
}

static void playerDeserialize(struct Serializer* serializer, struct Player* player) {
    struct Location location;
    transformInitIdentity(&location.transform);

    serializeRead(serializer, &location.transform, sizeof(struct PartialTransform));
    serializeRead(serializer, &player->body.velocity, sizeof(player->body.velocity));
    serializeRead(serializer, &location.roomIndex, sizeof(location.roomIndex));
    serializeRead(serializer, &player->flags, sizeof(player->flags));
    serializeRead(serializer, &player->health, sizeof(player->health));
    serializeRead(serializer, &player->grabbingThroughPortal, sizeof(player->grabbingThroughPortal));

    playerSetLocation(player, &location);
}

#define PORTAL_FLAGS_NO_PORTAL  -1

static void sceneSerializePortals(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int portalIndex = 0; portalIndex < 2; ++portalIndex) {
        if (!gCollisionScene.portalTransforms[portalIndex]) {
            char flags = PORTAL_FLAGS_NO_PORTAL;
            action(serializer, &flags, sizeof(flags));
            continue;
        }

        struct Portal* portal = &scene->portals[portalIndex];
        char flags = portal->flags;
        action(serializer, &flags, sizeof(flags));

        action(serializer, &portal->rigidBody.transform, sizeof(struct PartialTransform));
        action(serializer, &portal->portalSurfaceIndex, sizeof(portal->portalSurfaceIndex));
        action(serializer, &portal->rigidBody.currentRoom, sizeof(portal->rigidBody.currentRoom));
        action(serializer, &portal->colliderIndex, sizeof(portal->colliderIndex));
        action(serializer, &portal->transformIndex, sizeof(portal->transformIndex));

        if (portal->transformIndex != NO_TRANSFORM_INDEX) {
            action(serializer, &portal->relativePos, sizeof(portal->relativePos));
            action(serializer, &portal->relativeRotation, sizeof(portal->relativeRotation));
        }
    }
}

#define PORTAL_FLAGS_TO_DESERIALIZE    (PortalFlagsPlayerPortal | PortalFlagsZOffset)

static void sceneDeserializePortals(struct Serializer* serializer, struct Scene* scene) {
    for (int portalIndex = 0; portalIndex < 2; ++portalIndex) {
        char flags;
        serializeRead(serializer, &flags, sizeof(flags));

        if (flags == PORTAL_FLAGS_NO_PORTAL) {
            continue;
        }

        struct Portal* portal = &scene->portals[portalIndex];

        struct Transform transform;
        serializeRead(serializer, &transform, sizeof(struct PartialTransform));  
        transform.scale = gOneVec;

        short portalSurfaceIndex;
        short roomIndex;
        short colliderIndex;
        serializeRead(serializer, &portalSurfaceIndex, sizeof(portalSurfaceIndex));
        serializeRead(serializer, &roomIndex, sizeof(roomIndex));
        serializeRead(serializer, &colliderIndex, sizeof(colliderIndex));

        struct PortalSurface* existingSurface = portalSurfaceGetOriginalSurface(portalSurfaceIndex, portalIndex);

        portalAttachToSurface(
            portal, 
            existingSurface, 
            portalSurfaceIndex, 
            &transform,
            0,
            portalIndex
        );

        serializeRead(serializer, &portal->transformIndex, sizeof(portal->transformIndex));
        if (portal->transformIndex != NO_TRANSFORM_INDEX) {
            serializeRead(serializer, &portal->relativePos, sizeof(portal->relativePos));
            serializeRead(serializer, &portal->relativeRotation, sizeof(portal->relativeRotation));
        }

        portal->rigidBody.transform = transform;
        gCollisionScene.portalVelocity[portalIndex] = gZeroVec;
        portal->rigidBody.currentRoom = roomIndex;
        portal->colliderIndex = colliderIndex;
        portal->scale = 1.0f;
        collisionSceneSetPortal(portalIndex, &portal->rigidBody.transform, roomIndex, colliderIndex);
        collisionObjectUpdateBB(&portal->collisionObject);

        portal->flags &= ~PORTAL_FLAGS_TO_DESERIALIZE;
        portal->flags |= PORTAL_FLAGS_TO_DESERIALIZE & flags;

        portal->opacity = 0.0f;
    }
}   

static void buttonsSerializeRW(struct Serializer* serializer, SerializeAction action, struct Button* buttons, int count) {
    for (int i = 0; i < count; ++i) {
        action(serializer, &buttons[i].rigidBody.transform.position.y, sizeof(float));
        action(serializer, &buttons[i].state, sizeof(enum ButtonState));
    }
}

static void rigidBodyDeserializeFlags(enum RigidBodyFlags* flags) {
    if (*flags & RigidBodyForceWakeOnLoad) {
        *flags &= ~(RigidBodyIsSleeping | RigidBodyForceWakeOnLoad);
    } else {
        *flags &= ~RigidBodyHasWoken;
    }
}

static void rigidBodySerialize(struct Serializer* serializer, SerializeAction action, struct RigidBody* rigidBody) {
    action(serializer, &rigidBody->transform, sizeof(struct PartialTransform));
    action(serializer, &rigidBody->currentRoom, sizeof(short));
    action(serializer, &rigidBody->flags, sizeof(enum RigidBodyFlags));

    if (!(rigidBody->flags & RigidBodyIsSleeping)) {
        action(serializer, &rigidBody->velocity, sizeof(struct Vector3));
        action(serializer, &rigidBody->angularVelocity, sizeof(struct Vector3));
    }
}

static void rigidBodyDeserialize(struct Serializer* serializer, struct RigidBody* rigidBody) {
    serializeRead(serializer, &rigidBody->transform, sizeof(struct PartialTransform));
    serializeRead(serializer, &rigidBody->currentRoom, sizeof(short));
    serializeRead(serializer, &rigidBody->flags, sizeof(enum RigidBodyFlags));

    enum RigidBodyFlags serializedFlags = rigidBody->flags;
    rigidBodyDeserializeFlags(&rigidBody->flags);

    if (!(serializedFlags & RigidBodyIsSleeping)) {
        serializeRead(serializer, &rigidBody->velocity, sizeof(struct Vector3));
        serializeRead(serializer, &rigidBody->angularVelocity, sizeof(struct Vector3));
    } else {
        rigidBody->velocity = gZeroVec;
        rigidBody->angularVelocity = gZeroVec;
    }
}

static void decorSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    short countAsShort = 0;
    short heldObject = -1;

    for (int i = 0; i < scene->decorCount; ++i) {
        struct DecorObject* entry = scene->decor[i];
        if (entry->definition->colliderType.type == CollisionShapeTypeNone) {
            // Non-moving objects are loaded from the level definition
            continue;
        }

        if (playerIsGrabbingObject(&scene->player, &entry->collisionObject)) {
            heldObject = countAsShort;
        }
        
        ++countAsShort;
    }

    action(serializer, &countAsShort, sizeof(short));
    action(serializer, &heldObject, sizeof(short));

    for (int i = 0; i < scene->decorCount; ++i) {
        struct DecorObject* entry = scene->decor[i];
        if (entry->definition->colliderType.type == CollisionShapeTypeNone) {
            // Non-moving objects are loaded from the level definition
            continue;
        }

        short id = decorIdForObjectDefinition(entry->definition);

        action(serializer, &id, sizeof(short));

        if (entry->definition->flags & DecorObjectFlagsImportant) {
            // Non-important decor will never be reset, so don't serialize this
            action(serializer, &entry->originalPosition, sizeof(struct Vector3));
            action(serializer, &entry->originalRotation, sizeof(struct Quaternion));
            action(serializer, &entry->originalRoom, sizeof(short));
        }

        if (entry->definition->soundClipId != -1) {
            uint8_t muted = soundPlayerIsMuted(entry->playingSound);
            action(serializer, &muted, sizeof(uint8_t));
        }

        rigidBodySerialize(serializer, action, &entry->rigidBody);
        if (entry->rigidBody.flags & RigidBodyFizzled) {
            action(serializer, &entry->fizzleTime, sizeof(float));
        }
    }
}

static void decorDeserialize(struct Serializer* serializer, struct Scene* scene, struct LevelDefinition* level) {
    assert(scene->decorCount == 0);
    assert(scene->decor == NULL);

    short unserializedCount = 0;
    for (int i = 0; i < level->decorCount; ++i) {
        struct DecorDefinition* decorDef = &level->decor[i];
        struct DecorObjectDefinition* def = decorObjectDefinitionForId(decorDef->decorId);

        if (def->colliderType.type == CollisionShapeTypeNone) {
            ++unserializedCount;
        }
    }

    short serializedCount;
    serializeRead(serializer, &serializedCount, sizeof(short));
    short heldObject;
    serializeRead(serializer, &heldObject, sizeof(short));

    scene->decor = malloc(sizeof(struct DecorObject*) * (unserializedCount + serializedCount));
    scene->decorCount = 0;

    for (int i = 0; i < serializedCount; ++i) {
        short id;
        serializeRead(serializer, &id, sizeof(short));

        struct Transform originalTransform;
        short originalRoom;

        struct DecorObjectDefinition* definition = decorObjectDefinitionForId(id);
        if (definition->flags & DecorObjectFlagsImportant) {
            serializeRead(serializer, &originalTransform.position, sizeof(struct Vector3));
            serializeRead(serializer, &originalTransform.rotation, sizeof(struct Quaternion));
            originalTransform.scale = gOneVec;
            serializeRead(serializer, &originalRoom, sizeof(short));
        } else {
            transformInitIdentity(&originalTransform);
            originalRoom = 0;
        }

        uint8_t muted = 0;
        if (definition->soundClipId != -1) {
            serializeRead(serializer, &muted, sizeof(uint8_t));
        }
        if (muted) {
            definition->flags |= DecorObjectFlagsMuted;
        } else {
            definition->flags &= ~DecorObjectFlagsMuted;
        }

        struct DecorObject* entry = decorObjectNew(definition, &originalTransform, originalRoom);

        rigidBodyDeserialize(serializer, &entry->rigidBody);
        if (entry->rigidBody.flags & RigidBodyFizzled) {
            serializeRead(serializer, &entry->fizzleTime, sizeof(float));
        } else {
            entry->fizzleTime = 0.0f;
        }

        collisionObjectUpdateBB(&entry->collisionObject);

        scene->decor[scene->decorCount++] = entry;

        if (heldObject == i) {
            playerSetGrabbing(&scene->player, &entry->collisionObject);
        }

        decorObjectOnDeserialize(entry);
    }

    for (int i = 0; i < level->decorCount; ++i) {
        struct DecorDefinition* decorDef = &level->decor[i];
        struct DecorObjectDefinition* def = decorObjectDefinitionForId(decorDef->decorId);

        if (def->colliderType.type != CollisionShapeTypeNone) {
            // Dynamic objects are serialized
            continue;
        }

        struct Transform decorTransform;
        decorTransform.position = decorDef->position;
        decorTransform.rotation = decorDef->rotation;
        decorTransform.scale = gOneVec;
        scene->decor[scene->decorCount++] = decorObjectNew(def, &decorTransform, decorDef->roomIndex);
    }

    assert(scene->decorCount == (unserializedCount + serializedCount));
}

static void boxDropperSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    short heldCube = -1;
    for (int i = 0; i < scene->boxDropperCount; ++i) {
        if (playerIsGrabbingObject(&scene->player, &scene->boxDroppers[i].activeCube.collisionObject)) {
            heldCube = i;
            break;
        }
    }

    action(serializer, &heldCube, sizeof(short));

    for (int i = 0; i < scene->boxDropperCount; ++i) {
        struct BoxDropper* dropper = &scene->boxDroppers[i];
        action(serializer, &dropper->flags, sizeof(short));
        action(serializer, &dropper->reloadTimer, sizeof(float));

        if (!(dropper->flags & BoxDropperFlagsCubeIsActive)) {
            continue;
        }

        rigidBodySerialize(serializer, action, &dropper->activeCube.rigidBody);
        if (dropper->activeCube.rigidBody.flags & RigidBodyFizzled) {
            action(serializer, &dropper->activeCube.fizzleTime, sizeof(float));
        }
    }
}

static void boxDropperDeserialize(struct Serializer* serializer, struct Scene* scene) {
    short heldCube;
    serializeRead(serializer, &heldCube, sizeof(short));

    for (int i = 0; i < scene->boxDropperCount; ++i) {
        struct BoxDropper* dropper = &scene->boxDroppers[i];
        serializeRead(serializer, &dropper->flags, sizeof(short));
        serializeRead(serializer, &dropper->reloadTimer, sizeof(float));

        if (!(dropper->flags & BoxDropperFlagsCubeIsActive)) {
            continue;
        }

        // Cube droppers respawn their own cubes, so they are not marked as
        // important and therefore will never auto-reset. We still need to
        // fill in this data though for the initial spawn point.
        struct Transform dummyOriginalCubePosition;
        short dummyOriginalCubeRoom = 0;
        transformInitIdentity(&dummyOriginalCubePosition);

        decorObjectInit(&dropper->activeCube, dropper->cubeDef, &dummyOriginalCubePosition, dummyOriginalCubeRoom);

        rigidBodyDeserialize(serializer, &dropper->activeCube.rigidBody);
        collisionObjectUpdateBB(&dropper->activeCube.collisionObject);

        if (dropper->activeCube.rigidBody.flags & RigidBodyFizzled) {
            serializeRead(serializer, &dropper->activeCube.fizzleTime, sizeof(float));
        } else {
            dropper->activeCube.fizzleTime = 0.0f;
        }

        if (heldCube == i) {
            playerSetGrabbing(&scene->player, &dropper->activeCube.collisionObject);
        }

        decorObjectOnDeserialize(&dropper->activeCube);
    }
}

static void elevatorSerializeRW(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->elevatorCount; ++i) {
        action(serializer, &scene->elevators[i].flags, sizeof(short));
        action(serializer, &scene->elevators[i].timer, sizeof(float));
    }
}

static void pedestalSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->pedestalCount; ++i) {
        action(serializer, &scene->pedestals[i].flags, sizeof(short));
        action(serializer, &scene->pedestals[i].targetRotation, sizeof(struct Vector2));
        action(serializer, &scene->pedestals[i].currentRotation, sizeof(struct Vector2));
    }
}

static void pedestalDeserialize(struct Serializer* serializer, struct Scene* scene) {
    for (int i = 0; i < scene->pedestalCount; ++i) {
        serializeRead(serializer, &scene->pedestals[i].flags, sizeof(short));
        serializeRead(serializer, &scene->pedestals[i].targetRotation, sizeof(struct Vector2));
        serializeRead(serializer, &scene->pedestals[i].currentRotation, sizeof(struct Vector2));

        if (scene->pedestals[i].flags & PedestalFlagsDown) {
            pedestalSetDown(&scene->pedestals[i]);
        }
    }
}

static void launcherSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->ballLauncherCount; ++i) {
        struct BallLauncher* launcher = &scene->ballLaunchers[i];
        action(serializer, &launcher->currentBall.targetSpeed, sizeof(float));
        action(serializer, &launcher->currentBall.flags, sizeof(short));

        if (!ballIsActive(&launcher->currentBall) || ballIsCaught(&launcher->currentBall)) {
            continue;
        }
    
        action(serializer, &launcher->currentBall.rigidBody.transform.position, sizeof (struct Vector3));
        action(serializer, &launcher->currentBall.rigidBody.velocity, sizeof (struct Vector3));
        action(serializer, &launcher->currentBall.rigidBody.currentRoom, sizeof(short));
        action(serializer, &launcher->ballLifetime, sizeof(float));
    }
}

static void launcherDeserialize(struct Serializer* serializer, struct Scene* scene) {
    for (int i = 0; i < scene->ballLauncherCount; ++i) {
        struct BallLauncher* launcher = &scene->ballLaunchers[i];
        serializeRead(serializer, &launcher->currentBall.targetSpeed, sizeof(float));
        serializeRead(serializer, &launcher->currentBall.flags, sizeof(short));

        if (!ballIsActive(&launcher->currentBall) || ballIsCaught(&launcher->currentBall)) {
            continue;
        }

        struct Vector3 position;
        struct Vector3 velocity;
        short currentRoom;
        float lifetime;
        serializeRead(serializer, &position, sizeof (struct Vector3));
        serializeRead(serializer, &velocity, sizeof (struct Vector3));
        serializeRead(serializer, &currentRoom, sizeof(short));
        serializeRead(serializer, &lifetime, sizeof(float));

        ballInit(&launcher->currentBall, &position, &velocity, currentRoom, lifetime);
    }
}

static void catcherSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->ballCatcherCount; ++i) {
        struct BallCatcher* catcher = &scene->ballCatchers[i];

        short caughtIndex = -1;

        for (int launcherIndex = 0; launcherIndex < scene->ballLauncherCount; ++launcherIndex) {
            if (&scene->ballLaunchers[launcherIndex].currentBall == catcher->caughtBall) {
                caughtIndex = launcherIndex;
                break;
            }
        }

        action(serializer, &caughtIndex, sizeof(short));
    }
}

static void catcherDeserialize(struct Serializer* serializer, struct Scene* scene) {
    for (int i = 0; i < scene->ballCatcherCount; ++i) {
        short caughtIndex;
        serializeRead(serializer, &caughtIndex, sizeof(short));

        if (caughtIndex == -1) {
            continue;
        }

        struct BallCatcher* catcher = &scene->ballCatchers[i];
        struct Ball* caughtBall = &scene->ballLaunchers[caughtIndex].currentBall;

        ballInit(
            caughtBall,
            &catcher->rigidBody.transform.position,
            &gZeroVec,
            catcher->rigidBody.currentRoom,
            0.0f
        );
        ballCatcherHandBall(catcher, caughtBall);
    }
}

static void sceneAnimatorSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->animator.animatorCount; ++i) {
        action(serializer, &scene->animator.state[i].playbackSpeed, sizeof(float));

        struct SKArmature* armature = &scene->animator.armatures[i];
        for (int boneIndex = 0; boneIndex < armature->numberOfBones; ++boneIndex) {
            action(serializer, &armature->pose[boneIndex], sizeof(struct PartialTransform));
        }

        struct SKAnimator* animator = &scene->animator.animators[i];

        short animationIndex = -1;

        if (animator->currentClip) {
            animationIndex = animator->currentClip - scene->animator.animationInfo[i].clips;
        }

        action(serializer, &animationIndex, sizeof(short));

        if (animationIndex != -1) {
            action(serializer, &animator->currentTime, sizeof(float));
            action(serializer, &animator->flags, sizeof(short));
        }
    }
}

static void switchSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->switchCount; ++i) {
        struct Switch* switchObj = &scene->switches[i];

        action(serializer, &switchObj->timeLeft, sizeof(switchObj->timeLeft));
    }
}

static void signageSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->signageCount; ++i) {
        struct Signage* signage = &scene->signage[i];
        action(serializer, &signage->currentFrame, sizeof(signage->currentFrame));
    }
}

static void sceneAnimatorDeserialize(struct Serializer* serializer, struct Scene* scene) {
    for (int i = 0; i < scene->animator.animatorCount; ++i) {
        serializeRead(serializer, &scene->animator.state[i].playbackSpeed, sizeof(float));

        struct SKArmature* armature = &scene->animator.armatures[i];
        for (int boneIndex = 0; boneIndex < armature->numberOfBones; ++boneIndex) {
            serializeRead(serializer, &armature->pose[boneIndex], sizeof(struct PartialTransform));
        }

        struct SKAnimator* animator = &scene->animator.animators[i];

        short animationIndex = -1;
        serializeRead(serializer, &animationIndex, sizeof(short));

        if (animationIndex != -1) {
            float time;
            short flags;
            serializeRead(serializer, &time, sizeof(float));
            serializeRead(serializer, &flags, sizeof(short));

            skAnimatorRunClip(animator, &scene->animator.animationInfo[i].clips[animationIndex], time, flags);
        }
    }
}

static void securityCameraSerializeSingle(struct Serializer* serializer, SerializeAction action, struct SecurityCamera* securityCamera) {
    action(serializer, &securityCamera->index, sizeof(uint8_t));

    rigidBodySerialize(serializer, action, &securityCamera->rigidBody);
    if (securityCamera->rigidBody.flags & RigidBodyFizzled) {
        action(serializer, &securityCamera->fizzleTime, sizeof(float));
    }
}

static void securityCameraSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene, struct LevelDefinition* level) {
    uint8_t serializedCount = (level->securityCameraCount - scene->securityCameraCount);
    int16_t heldCam = -1;
    for (int i = 0; i < scene->securityCameraCount; ++i) {
        struct SecurityCamera* cam = scene->securityCameras[i];
        if (securityCameraIsDetached(cam)) {
            if (playerIsGrabbingObject(&scene->player, &cam->collisionObject)) {
                heldCam = cam->index;
            }
            ++serializedCount;
        }
    }
    action(serializer, &serializedCount, sizeof(uint8_t));
    action(serializer, &heldCam, sizeof(int16_t));

    // For serializing cameras that have already been removed from the scene
    struct SecurityCamera deadCam;
    transformInitIdentity(&deadCam.rigidBody.transform);
    deadCam.rigidBody.currentRoom = RIGID_BODY_NO_ROOM;
    deadCam.rigidBody.flags = RigidBodyFizzled | RigidBodyIsSleeping;
    deadCam.fizzleTime = 1.0f;

    int lastCamIndex = -1;
    for (int i = 0; i < scene->securityCameraCount; ++i) {
        struct SecurityCamera* cam = scene->securityCameras[i];

        // Serialize already-fizzled cameras
        for (int gapIdx = lastCamIndex + 1; gapIdx < cam->index; ++gapIdx) {
            deadCam.index = gapIdx;
            securityCameraSerializeSingle(serializer, action, &deadCam);
        }

        if (securityCameraIsDetached(cam)) {
            securityCameraSerializeSingle(serializer, action, cam);
        }

        lastCamIndex = cam->index;
    }

    // Serialize already-fizzled cameras
    for (int i = lastCamIndex + 1; i < level->securityCameraCount; ++i) {
        deadCam.index = i;
        securityCameraSerializeSingle(serializer, action, &deadCam);
    }
}

static void securityCameraDeserialize(struct Serializer* serializer, struct Scene* scene) {
    uint8_t serializedCount;
    serializeRead(serializer, &serializedCount, sizeof(uint8_t));

    int16_t heldCam;
    serializeRead(serializer, &heldCam, sizeof(int16_t));
    
    for (int i = 0; i < serializedCount; ++i) {
        uint8_t index;
        serializeRead(serializer, &index, sizeof(uint8_t));
        if (index >= scene->securityCameraCount) {
            continue;
        }
        
        struct SecurityCamera* cam = scene->securityCameras[index];
        
        securityCameraDetach(cam);

        rigidBodyDeserialize(serializer, &cam->rigidBody);
        if (cam->rigidBody.flags & RigidBodyFizzled) {
            serializeRead(serializer, &cam->fizzleTime, sizeof(float));
        } else {
            cam->fizzleTime = 0.0f;
        }

        collisionObjectUpdateBB(&cam->collisionObject);

        if (cam->index == heldCam) {
            playerSetGrabbing(&scene->player, &cam->collisionObject);
        }

        securityCameraOnDeserialize(cam);
    }
}

static void turretSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    short heldObject = -1;

    for (int i = 0; i < scene->turretCount; ++i) {
        struct Turret* turret = scene->turrets[i];

        if (playerIsGrabbingObject(&scene->player, &turret->collisionObject)) {
            heldObject = i;
            break;
        }
    }

    action(serializer, &scene->turretCount, sizeof(u8));
    action(serializer, &heldObject, sizeof(short));

    for (int i = 0; i < scene->turretCount; ++i) {
        struct Turret* turret = scene->turrets[i];

        rigidBodySerialize(serializer, action, &turret->rigidBody);
        if (turret->rigidBody.flags & RigidBodyFizzled) {
            action(serializer, &turret->fizzleTime, sizeof(float));
        }

        action(serializer, &turret->flags, sizeof(enum TurretFlags));
        action(serializer, &turret->state, sizeof(enum TurretState));

        if (turret->state != TurretStateDead) {
            action(serializer, &turret->stateTimer, sizeof(float));
            action(serializer, &turret->playerDetectTimer, sizeof(float));

            if (turret->state != TurretStateIdle) {
                action(serializer, turretGetLookRotation(turret), sizeof(struct Quaternion));
                action(serializer, &turret->targetRotation, sizeof(struct Quaternion));
                action(serializer, &turret->rotationSpeed, sizeof(float));

                action(serializer, &turret->openAmount, sizeof(float));
                action(serializer, &turret->stateData, sizeof(union TurretStateData));
            }
        }
    }
}

static void turretDeserialize(struct Serializer* serializer, struct Scene* scene) {
    assert(scene->turretCount == 0);
    assert(scene->turrets == NULL);

    u8 count;
    serializeRead(serializer, &count, sizeof(u8));
    short heldObject;
    serializeRead(serializer, &heldObject, sizeof(short));

    scene->turrets = malloc(sizeof(struct Turret*) * count);
    scene->turretCount = count;

    for (int i = 0; i < count; ++i) {
        struct Turret* turret = turretNew(NULL);

        rigidBodyDeserialize(serializer, &turret->rigidBody);
        if (turret->rigidBody.flags & RigidBodyFizzled) {
            serializeRead(serializer, &turret->fizzleTime, sizeof(float));
        } else {
            turret->fizzleTime = 0.0f;
        }

        serializeRead(serializer, &turret->flags, sizeof(enum TurretFlags));
        serializeRead(serializer, &turret->state, sizeof(enum TurretState));

        collisionObjectUpdateBB(&turret->collisionObject);

        if (heldObject == i) {
            playerSetGrabbing(&scene->player, &turret->collisionObject);
        }

        if (turret->state != TurretStateDead) {
            serializeRead(serializer, &turret->stateTimer, sizeof(float));
            serializeRead(serializer, &turret->playerDetectTimer, sizeof(float));

            if (turret->state != TurretStateIdle) {
                serializeRead(serializer, turretGetLookRotation(turret), sizeof(struct Quaternion));
                serializeRead(serializer, &turret->targetRotation, sizeof(struct Quaternion));
                serializeRead(serializer, &turret->rotationSpeed, sizeof(float));

                serializeRead(serializer, &turret->openAmount, sizeof(float));
                serializeRead(serializer, &turret->stateData, sizeof(union TurretStateData));
            }
        }

        turretOnDeserialize(turret);
        scene->turrets[i] = turret;
    }
}

static void namedCollisionSerialize(struct Serializer* serializer, SerializeAction action, struct LevelDefinition* level) {
    for (int i = 0; i < level->namedColliderCount; ++i) {
        short quadIndex = level->namedColliderIndices[i];
        struct CollisionObject* quad = &level->collisionQuads[quadIndex];

        action(serializer, &quad->collisionLayers, sizeof(u16));
    }
}

#define INCLUDE_SAVEFILE_ALIGN_CHECKS   0

#if INCLUDE_SAVEFILE_ALIGN_CHECKS
#define WRITE_ALIGN_CHECK   {action(serializer, &currentAlign, 1); ++currentAlign;}
#define READ_ALIGN_CHECK {serializeRead(serializer, &currentAlign, 1); if (currentAlign != expectedAlign) debug_assert(0); ++expectedAlign;}
#else
#define WRITE_ALIGN_CHECK
#define READ_ALIGN_CHECK
#endif

void sceneSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
#if INCLUDE_SAVEFILE_ALIGN_CHECKS
    char currentAlign = 0;
#endif
    playerSerialize(serializer, action, &scene->player);
    WRITE_ALIGN_CHECK;
    sceneSerializePortals(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    buttonsSerializeRW(serializer, action, scene->buttons, scene->buttonCount);
    WRITE_ALIGN_CHECK;
    decorSerialize(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    boxDropperSerialize(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    elevatorSerializeRW(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    pedestalSerialize(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    signageSerialize(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    launcherSerialize(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    catcherSerialize(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    sceneAnimatorSerialize(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    switchSerialize(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    securityCameraSerialize(serializer, action, scene, gCurrentLevel);
    WRITE_ALIGN_CHECK;
    turretSerialize(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    namedCollisionSerialize(serializer, action, gCurrentLevel);
    WRITE_ALIGN_CHECK;
}

void sceneDeserialize(struct Serializer* serializer, struct Scene* scene) {
#if INCLUDE_SAVEFILE_ALIGN_CHECKS
    char currentAlign = 0;
    char expectedAlign = 0;
#endif
    playerDeserialize(serializer, &scene->player);
    READ_ALIGN_CHECK;
    sceneDeserializePortals(serializer, scene);
    READ_ALIGN_CHECK;
    buttonsSerializeRW(serializer, serializeRead, scene->buttons, scene->buttonCount);
    READ_ALIGN_CHECK;
    decorDeserialize(serializer, scene, gCurrentLevel);
    READ_ALIGN_CHECK;
    boxDropperDeserialize(serializer, scene);
    READ_ALIGN_CHECK;
    elevatorSerializeRW(serializer, serializeRead, scene);
    READ_ALIGN_CHECK;
    pedestalDeserialize(serializer, scene);
    READ_ALIGN_CHECK;
    signageSerialize(serializer, serializeRead, scene);
    READ_ALIGN_CHECK;
    launcherDeserialize(serializer, scene);
    READ_ALIGN_CHECK;
    catcherDeserialize(serializer, scene);
    READ_ALIGN_CHECK;
    sceneAnimatorDeserialize(serializer, scene);
    READ_ALIGN_CHECK;
    switchSerialize(serializer, serializeRead, scene);
    READ_ALIGN_CHECK;
    securityCameraDeserialize(serializer, scene);
    READ_ALIGN_CHECK;
    turretDeserialize(serializer, scene);
    READ_ALIGN_CHECK;
    namedCollisionSerialize(serializer, serializeRead, gCurrentLevel);
    READ_ALIGN_CHECK;

    for (int i = 0; i < scene->doorCount; ++i) {
        doorOnDeserialize(&scene->doors[i]);
    }

    for (int i = 0; i < scene->incineratorCount; ++i) {
        incineratorOnDeserialize(&scene->incinerators[i]);
    }

    portalGunOnDeserialize(&scene->portalGun, &scene->player);
}

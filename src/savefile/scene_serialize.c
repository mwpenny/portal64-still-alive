#include "scene_serialize.h"

#include <assert.h>

#include "decor/decor_object_list.h"
#include "levels/levels.h"
#include "physics/collision_scene.h"
#include "util/memory.h"

void playerSerialize(struct Serializer* serializer, SerializeAction action, struct Player* player) {
    action(serializer, &player->lookTransform, sizeof(struct PartialTransform));
    action(serializer, &player->body.velocity, sizeof(player->body.velocity));
    action(serializer, &player->body.currentRoom, sizeof(player->body.currentRoom));
    action(serializer, &player->flags, sizeof(player->flags));
    action(serializer, &player->health, sizeof(player->health));
    action(serializer, &player->grabbingThroughPortal, sizeof(player->grabbingThroughPortal));
}

void playerDeserialize(struct Serializer* serializer, struct Player* player) {
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

void sceneSerializePortals(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
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

void sceneDeserializePortals(struct Serializer* serializer, struct Scene* scene) {
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

void buttonsSerializeRW(struct Serializer* serializer, SerializeAction action, struct Button* buttons, int count) {
    for (int i = 0; i < count; ++i) {
        action(serializer, &buttons[i].rigidBody.transform.position.y, sizeof(float));
        action(serializer, &buttons[i].flags, sizeof(buttons[i].flags));
    }
}

void decorSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    short countAsShort = 0;
    short heldObject = -1;

    for (int i = 0; i < scene->decorCount; ++i) {
        struct DecorObject* entry = scene->decor[i];
        if (entry->definition->colliderType.type == CollisionShapeTypeNone) {
            // non moving objects can be loaded from the level definition
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
            // non moving objects can be loaded from the level definition
            continue;
        }

        short id = decorIdForObjectDefinition(entry->definition);

        action(serializer, &id, sizeof(short));

        action(serializer, &entry->originalPosition, sizeof(struct Vector3));
        action(serializer, &entry->originalRotation, sizeof(struct Quaternion));
        action(serializer, &entry->originalRoom, sizeof(short));

        action(serializer, &entry->rigidBody.transform, sizeof(struct PartialTransform));
        action(serializer, &entry->rigidBody.velocity, sizeof(struct Vector3));
        action(serializer, &entry->rigidBody.angularVelocity, sizeof(struct Vector3));
        action(serializer, &entry->rigidBody.flags, sizeof(enum RigidBodyFlags));
        action(serializer, &entry->rigidBody.currentRoom, sizeof(short));

        action(serializer, &entry->fizzleTime, sizeof(float));

        entry->rigidBody.flags &= ~RigidBodyIsSleeping;
        entry->rigidBody.sleepFrames = IDLE_SLEEP_FRAMES;
    }
}

void decorDeserialize(struct Serializer* serializer, struct Scene* scene) {
    assert(scene->decorCount == 0);
    assert(scene->decor == NULL);

    short countAsShort;
    serializeRead(serializer, &countAsShort, sizeof(short));
    short heldObject;
    serializeRead(serializer, &heldObject, sizeof(short));

    scene->decor = malloc(sizeof(struct DecorObject*) * (countAsShort + gCurrentLevel->decorCount));

    for (int i = 0; i < countAsShort; ++i) {
        short id;

        serializeRead(serializer, &id, sizeof(short));

        struct Transform transform;
        short originalRoom;

        serializeRead(serializer, &transform.position, sizeof(struct Vector3));
        serializeRead(serializer, &transform.rotation, sizeof(struct Quaternion));
        transform.scale = gOneVec;
        serializeRead(serializer, &originalRoom, sizeof(short));

        struct DecorObject* entry = decorObjectNew(decorObjectDefinitionForId(id), &transform, originalRoom);

        serializeRead(serializer, &entry->rigidBody.transform, sizeof(struct PartialTransform));
        serializeRead(serializer, &entry->rigidBody.velocity, sizeof(struct Vector3));
        serializeRead(serializer, &entry->rigidBody.angularVelocity, sizeof(struct Vector3));
        serializeRead(serializer, &entry->rigidBody.flags, sizeof(enum RigidBodyFlags));
        serializeRead(serializer, &entry->rigidBody.currentRoom, sizeof(short));
        collisionObjectUpdateBB(&entry->collisionObject);

        serializeRead(serializer, &entry->fizzleTime, sizeof(float));

        scene->decor[i] = entry;

        if (heldObject == i) {
            playerSetGrabbing(&scene->player, &entry->collisionObject);
        }
    }

    for (int i = 0; i < gCurrentLevel->decorCount; ++i) {
        struct DecorDefinition* decorDef = &gCurrentLevel->decor[i];
        struct DecorObjectDefinition* def = decorObjectDefinitionForId(decorDef->decorId);

        if (def->colliderType.type != CollisionShapeTypeNone) {
            // dynamic objects are serialized
            continue;
        }

        struct Transform decorTransform;
        decorTransform.position = decorDef->position;
        decorTransform.rotation = decorDef->rotation;
        decorTransform.scale = gOneVec;
        scene->decor[countAsShort] = decorObjectNew(def, &decorTransform, decorDef->roomIndex);
        ++countAsShort;
    }

    scene->decorCount = countAsShort;
}

void boxDropperSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
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

        action(serializer, &dropper->activeCube.rigidBody.transform, sizeof(struct PartialTransform));
        action(serializer, &dropper->activeCube.rigidBody.currentRoom, sizeof(short));
        action(serializer, &dropper->activeCube.rigidBody.velocity, sizeof(struct Vector3));
        action(serializer, &dropper->activeCube.rigidBody.angularVelocity, sizeof(struct Vector3));
        action(serializer, &dropper->activeCube.rigidBody.flags, sizeof(enum RigidBodyFlags));
        action(serializer, &dropper->activeCube.fizzleTime, sizeof(float));
    }
}

void boxDropperDeserialize(struct Serializer* serializer, struct Scene* scene) {
    short heldCube;
    serializeRead(serializer, &heldCube, sizeof(short));

    for (int i = 0; i < scene->boxDropperCount; ++i) {
        struct BoxDropper* dropper = &scene->boxDroppers[i];
        serializeRead(serializer, &dropper->flags, sizeof(short));
        serializeRead(serializer, &dropper->reloadTimer, sizeof(float));

        if (!(dropper->flags & BoxDropperFlagsCubeIsActive)) {
            continue;
        }

        struct Transform cubePosition;
        short cubeRoom;
        serializeRead(serializer, &cubePosition, sizeof(struct PartialTransform));
        cubePosition.scale = gOneVec;
        serializeRead(serializer, &cubeRoom, sizeof(short));

        decorObjectInit(&dropper->activeCube, decorObjectDefinitionForId(DECOR_TYPE_CUBE_UNIMPORTANT), &cubePosition, cubeRoom);

        serializeRead(serializer, &dropper->activeCube.rigidBody.velocity, sizeof(struct Vector3));
        serializeRead(serializer, &dropper->activeCube.rigidBody.angularVelocity, sizeof(struct Vector3));
        serializeRead(serializer, &dropper->activeCube.rigidBody.flags, sizeof(enum RigidBodyFlags));
        serializeRead(serializer, &dropper->activeCube.fizzleTime, sizeof(float));

        dropper->activeCube.rigidBody.flags &= ~RigidBodyIsSleeping;
        dropper->activeCube.rigidBody.sleepFrames = IDLE_SLEEP_FRAMES;

        if (heldCube == i) {
            playerSetGrabbing(&scene->player, &dropper->activeCube.collisionObject);
        }
    }
}

void elevatorSerializeRW(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->elevatorCount; ++i) {
        action(serializer, &scene->elevators[i].flags, sizeof(short));
        action(serializer, &scene->elevators[i].timer, sizeof(float));
    }
}

void pedestalSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->pedestalCount; ++i) {
        action(serializer, &scene->pedestals[i].flags, sizeof(short));
        action(serializer, &scene->pedestals[i].pointAt, sizeof(struct Vector3));
        action(serializer, &scene->pedestals[i].currentRotation, sizeof(struct Vector2));
    }
}

void pedestalDeserialize(struct Serializer* serializer, struct Scene* scene) {
    for (int i = 0; i < scene->pedestalCount; ++i) {
        serializeRead(serializer, &scene->pedestals[i].flags, sizeof(short));
        serializeRead(serializer, &scene->pedestals[i].pointAt, sizeof(struct Vector3));
        serializeRead(serializer, &scene->pedestals[i].currentRotation, sizeof(struct Vector2));

        if (scene->pedestals[i].flags & PedestalFlagsDown) {
            pedestalSetDown(&scene->pedestals[i]);
        }
    }
}

void launcherSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->ballLauncherCount; ++i) {
        struct BallLauncher* launcher = &scene->ballLaunchers[i];
        action(serializer, &launcher->currentBall.targetSpeed, sizeof(float));
        action(serializer, &launcher->currentBall.flags, sizeof(short));

        if (!ballIsActive(&launcher->currentBall) || ballIsCaught(&launcher->currentBall)) {
            continue;
        }
    
        action(serializer, &launcher->currentBall.rigidBody.transform.position, sizeof (struct Vector3));
        action(serializer, &launcher->currentBall.rigidBody.velocity, sizeof (struct Vector3));
        action(serializer, &launcher->rigidBody.currentRoom, sizeof(short));
        action(serializer, &launcher->ballLifetime, sizeof(float));
    }
}

void launcherDeserialize(struct Serializer* serializer, struct Scene* scene) {
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

void catcherSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
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

void catcherDeserialize(struct Serializer* serializer, struct Scene* scene) {
    for (int i = 0; i < scene->ballCatcherCount; ++i) {
        short caughtIndex;
        serializeRead(serializer, &caughtIndex, sizeof(short));

        if (caughtIndex == -1) {
            continue;
        }

        struct BallCatcher* catcher = &scene->ballCatchers[i];
        ballCatcherHandBall(catcher, &scene->ballLaunchers[caughtIndex].currentBall);
    }
}

void sceneAnimatorSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
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
        }
    }
}

void switchSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->switchCount; ++i) {
        struct Switch* switchObj = &scene->switches[i];

        action(serializer, &switchObj->timeLeft, sizeof(switchObj->timeLeft));
    }
}

void signageSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    for (int i = 0; i < scene->signageCount; ++i) {
        struct Signage* signage = &scene->signage[i];
        action(serializer, &signage->currentFrame, sizeof(signage->currentFrame));
    }
}

void sceneAnimatorDeserialize(struct Serializer* serializer, struct Scene* scene) {
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
            serializeRead(serializer, &time, sizeof(float));
            skAnimatorRunClip(animator, &scene->animator.animationInfo[i].clips[animationIndex], time, 0);
        }
    }
}

void securityCameraSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    u8 serializedCount = 0;
    short heldCam = -1;
    for (int i = 0; i < scene->securityCameraCount; ++i) {
        struct SecurityCamera* cam = &scene->securityCameras[i];
        if (securityCameraIsDetached(cam)) {
            if (playerIsGrabbingObject(&scene->player, &cam->collisionObject)) {
                heldCam = serializedCount;
            }
            ++serializedCount;
        }
    }
    action(serializer, &serializedCount, sizeof(u8));
    action(serializer, &heldCam, sizeof(short));
    
    for (int i = 0; i < scene->securityCameraCount; ++i) {
        struct SecurityCamera* cam = &scene->securityCameras[i];
        if (securityCameraIsDetached(cam)) {
            u8 index = i;
            action(serializer, &index, sizeof(u8));
            
            action(serializer, &cam->rigidBody.transform, sizeof(struct PartialTransform));
            action(serializer, &cam->rigidBody.velocity, sizeof(struct Vector3));
            action(serializer, &cam->rigidBody.angularVelocity, sizeof(struct Vector3));
            action(serializer, &cam->rigidBody.flags, sizeof(enum RigidBodyFlags));
            action(serializer, &cam->rigidBody.currentRoom, sizeof(short));

            action(serializer, &cam->fizzleTime, sizeof(float));
        }
    }
}

void securityCameraDeserialize(struct Serializer* serializer, struct Scene* scene) {
    u8 serializedCount;
    serializeRead(serializer, &serializedCount, sizeof(u8));
    
    short heldCam;
    serializeRead(serializer, &heldCam, sizeof(short));
    
    for (int i = 0; i < serializedCount; ++i) {
        u8 index;
        serializeRead(serializer, &index, sizeof(u8));
        if (index >= scene->securityCameraCount) {
            continue;
        }
        
        struct SecurityCamera* cam = &scene->securityCameras[index];
        
        securityCameraDetach(cam);
        
        serializeRead(serializer, &cam->rigidBody.transform, sizeof(struct PartialTransform));
        serializeRead(serializer, &cam->rigidBody.velocity, sizeof(struct Vector3));
        serializeRead(serializer, &cam->rigidBody.angularVelocity, sizeof(struct Vector3));
        serializeRead(serializer, &cam->rigidBody.flags, sizeof(enum RigidBodyFlags));
        serializeRead(serializer, &cam->rigidBody.currentRoom, sizeof(short));
        collisionObjectUpdateBB(&cam->collisionObject);

        serializeRead(serializer, &cam->fizzleTime, sizeof(float));
        
        cam->rigidBody.flags &= ~RigidBodyIsSleeping;
        cam->rigidBody.sleepFrames = IDLE_SLEEP_FRAMES;

        if (heldCam == i) {
            playerSetGrabbing(&scene->player, &cam->collisionObject);
        }
    }
}

void turretSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
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

        action(serializer, &turret->rigidBody.transform, sizeof(struct PartialTransform));
        action(serializer, &turret->rigidBody.velocity, sizeof(struct Vector3));
        action(serializer, &turret->rigidBody.angularVelocity, sizeof(struct Vector3));
        action(serializer, &turret->rigidBody.flags, sizeof(enum RigidBodyFlags));
        action(serializer, &turret->rigidBody.currentRoom, sizeof(short));

        action(serializer, &turret->state, sizeof(enum TurretState));
        action(serializer, &turret->fizzleTime, sizeof(float));

        if (turret->state != TurretStateDead) {
            action(serializer, &turret->stateTimer, sizeof(float));
            action(serializer, &turret->playerDetectTimer, sizeof(float));

            if (turret->state != TurretStateIdle) {
                action(serializer, turretGetLookRotation(turret), sizeof(struct Quaternion));
                action(serializer, &turret->targetRotation, sizeof(struct Quaternion));
                action(serializer, &turret->rotationSpeed, sizeof(float));

                action(serializer, &turret->flags, sizeof(enum TurretFlags));
                action(serializer, &turret->openAmount, sizeof(float));
                action(serializer, &turret->stateData, sizeof(union TurretStateData));
            }
        }
    }
}

void turretDeserialize(struct Serializer* serializer, struct Scene* scene) {
    assert(scene->turretCount == 0);
    assert(scene->turrets == NULL);

    u8 count;
    serializeRead(serializer, &count, sizeof(u8));
    short heldObject;
    serializeRead(serializer, &heldObject, sizeof(short));

    scene->turrets = malloc(sizeof(struct Turret*) * count);

    for (int i = 0; i < count; ++i) {
        struct Turret* turret = turretNew(NULL);

        serializeRead(serializer, &turret->rigidBody.transform, sizeof(struct PartialTransform));
        serializeRead(serializer, &turret->rigidBody.velocity, sizeof(struct Vector3));
        serializeRead(serializer, &turret->rigidBody.angularVelocity, sizeof(struct Vector3));
        serializeRead(serializer, &turret->rigidBody.flags, sizeof(enum RigidBodyFlags));
        serializeRead(serializer, &turret->rigidBody.currentRoom, sizeof(short));

        serializeRead(serializer, &turret->state, sizeof(enum TurretState));
        serializeRead(serializer, &turret->fizzleTime, sizeof(float));

        collisionObjectUpdateBB(&turret->collisionObject);
        turret->rigidBody.flags &= ~RigidBodyIsSleeping;
        turret->rigidBody.sleepFrames = IDLE_SLEEP_FRAMES;

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

                serializeRead(serializer, &turret->flags, sizeof(enum TurretFlags));
                serializeRead(serializer, &turret->openAmount, sizeof(float));
                serializeRead(serializer, &turret->stateData, sizeof(union TurretStateData));
            }
        }

        turretOnDeserialize(turret);
        scene->turrets[i] = turret;
    }

    scene->turretCount = count;
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
    securityCameraSerialize(serializer, action, scene);
    WRITE_ALIGN_CHECK;
    turretSerialize(serializer, action, scene);
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
    decorDeserialize(serializer, scene);
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

    for (int i = 0; i < scene->doorCount; ++i) {
        doorCheckForOpenState(&scene->doors[i]);
    }

    scene->hud.overlayTimer = 0.0f;

    if (scene->player.flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun)) {
        scene->portalGun.rotation = scene->player.lookTransform.rotation;
    }
}

#include "./scene_serialize.h"

#include "../physics/collision_scene.h"
#include "../decor/decor_object_list.h"
#include "../util/memory.h"
#include "../levels/levels.h"

void playerSerialize(struct Serializer* serializer, SerializeAction action, struct Player* player) {
    action(serializer, &player->lookTransform, sizeof(struct PartialTransform));
    action(serializer, &player->body.velocity, sizeof(player->body.velocity));
    action(serializer, &player->body.currentRoom, sizeof(player->body.currentRoom));
    action(serializer, &player->flags, sizeof(player->flags));
}

void playerDeserialize(struct Serializer* serializer, struct Player* player) {
    serializeRead(serializer, &player->lookTransform, sizeof(struct PartialTransform));
    player->body.transform.position = player->lookTransform.position;
    serializeRead(serializer, &player->body.velocity, sizeof(player->body.velocity));
    serializeRead(serializer, &player->body.currentRoom, sizeof(player->body.currentRoom));
    serializeRead(serializer, &player->flags, sizeof(player->flags));
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

        action(serializer, &portal->transform, sizeof(struct PartialTransform));
        action(serializer, &portal->portalSurfaceIndex, sizeof(portal->portalSurfaceIndex));
        action(serializer, &portal->roomIndex, sizeof(portal->roomIndex));
        action(serializer, &portal->transformIndex, sizeof(portal->transformIndex));

        if (portal->transformIndex != NO_TRANSFORM_INDEX) {
            action(serializer, &portal->relativePos, sizeof(portal->relativePos));
        }
    }
}

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
        serializeRead(serializer, &portalSurfaceIndex, sizeof(portalSurfaceIndex));
        serializeRead(serializer, &roomIndex, sizeof(roomIndex));

        struct PortalSurface* existingSurface = portalSurfaceGetOriginalSurface(portalSurfaceIndex, portalIndex);

        portalAttachToSurface(
            portal, 
            existingSurface, 
            portalSurfaceIndex, 
            &transform,
            0
        );

        serializeRead(serializer, &portal->transformIndex, sizeof(portal->transformIndex));
        if (portal->transformIndex != NO_TRANSFORM_INDEX) {
            serializeRead(serializer, &portal->relativePos, sizeof(portal->relativePos));
        }

        portal->transform = transform;
        gCollisionScene.portalVelocity[portalIndex] = gZeroVec;
        portal->roomIndex = roomIndex;
        portal->scale = 1.0f;
        gCollisionScene.portalTransforms[portalIndex] = &portal->transform;
        gCollisionScene.portalRooms[portalIndex] = roomIndex;

        if (flags & PortalFlagsPlayerPortal) {
            portal->flags |= PortalFlagsPlayerPortal;
        } else {
            portal->flags &= ~PortalFlagsPlayerPortal;
        }

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

        if (scene->player.grabConstraint.object == &entry->collisionObject) {
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
    }
}

void decorDeserialize(struct Serializer* serializer, struct Scene* scene) {
    if (scene->decor) {
        for (int i = 0; i < scene->decorCount; ++i) {
            decorObjectDelete(scene->decor[i]);
        }
        free(scene->decor);
    }

    scene->decorCount = 0;

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
        if (&scene->boxDroppers[i].activeCube.collisionObject == scene->player.grabConstraint.object) {
            heldCube = i;
            break;
        }
    }

    action(serializer, &heldCube, sizeof(short));

    for (int i = 0; i < scene->boxDropperCount; ++i) {
        struct BoxDropper* dropper = &scene->boxDroppers[i];
        action(serializer, &dropper->flags, sizeof(short));

        if (!(dropper->flags & BoxDropperFlagsCubeIsActive)) {
            continue;
        }

        action(serializer, &dropper->activeCube.rigidBody.transform, sizeof(struct PartialTransform));
        action(serializer, &dropper->activeCube.rigidBody.currentRoom, sizeof(short));
        action(serializer, &dropper->activeCube.rigidBody.velocity, sizeof(struct Vector3));
        action(serializer, &dropper->activeCube.rigidBody.angularVelocity, sizeof(struct Vector3));
        action(serializer, &dropper->activeCube.rigidBody.flags, sizeof(enum RigidBodyFlags));
    }
}

void boxDropperDeserialize(struct Serializer* serializer, struct Scene* scene) {
    short heldCube;
    serializeRead(serializer, &heldCube, sizeof(short));

    for (int i = 0; i < scene->boxDropperCount; ++i) {
        struct BoxDropper* dropper = &scene->boxDroppers[i];
        serializeRead(serializer, &dropper->flags, sizeof(short));

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

        if (heldCube == i) {
            playerSetGrabbing(&scene->player, &dropper->activeCube.collisionObject);
        }
    }
}

void sceneSerialize(struct Serializer* serializer, SerializeAction action, struct Scene* scene) {
    playerSerialize(serializer, action, &scene->player);
    sceneSerializePortals(serializer, action, scene);
    buttonsSerializeRW(serializer, action, scene->buttons, scene->buttonCount);
    decorSerialize(serializer, action, scene);
    boxDropperSerialize(serializer, action, scene);
}

void sceneDeserialize(struct Serializer* serializer, struct Scene* scene) {
    playerDeserialize(serializer, &scene->player);
    sceneDeserializePortals(serializer, scene);
    buttonsSerializeRW(serializer, serializeRead, scene->buttons, scene->buttonCount);
    decorDeserialize(serializer, scene);
    boxDropperDeserialize(serializer, scene);
}
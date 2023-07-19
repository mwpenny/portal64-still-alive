#include "collision_scene.h"

#include "math/mathf.h"
#include "gjk.h"
#include "epa.h"
#include "contact_solver.h"
#include "../util/memory.h"
#include "../scene/portal.h"
#include "../levels/levels.h"

struct CollisionScene gCollisionScene;

void collisionSceneInit(struct CollisionScene* scene, struct CollisionObject* quads, int quadCount, struct World* world) {
    scene->quads = quads;
    scene->quadCount = quadCount;

    scene->world = world;

    scene->dynamicObjectCount = 0;
    scene->portalTransforms[0] = NULL;
    scene->portalTransforms[1] = NULL;
    scene->portalColliderIndex[0] = -1;
    scene->portalColliderIndex[1] = -1;
}

int mergeColliderList(short* a, int aCount, short* b, int bCount, short* output) {
    int aIndex = 0;
    int bIndex = 0;
    int result = 0;

    while (aIndex < aCount && bIndex < bCount) {
        if (a[aIndex] == b[bIndex]) {
            output[result] = a[aIndex];
            ++result;
            ++aIndex;
            ++bIndex;
        } else if (a[aIndex] < b[bIndex]) {
            output[result] = a[aIndex];
            ++result;
            ++aIndex;
        } else {
            output[result] = b[bIndex];
            ++result;
            ++bIndex;
        }
    }

    while (aIndex < aCount) {
        output[result] = a[aIndex];
        ++result;
        ++aIndex;
    }
    
    while (bIndex < bCount) {
        output[result] = b[bIndex];
        ++result;
        ++bIndex;
    }

    return result;
}

#define MAX_COLLIDERS   64

#define COLLISION_GRID_CELL_SIZE  4

#define GRID_CELL_X(room, worldX) floorf(((worldX) - room->cornerX) * (1.0f / COLLISION_GRID_CELL_SIZE));
#define GRID_CELL_Z(room, worldZ) floorf(((worldZ) - room->cornerZ) * (1.0f / COLLISION_GRID_CELL_SIZE));

#define GRID_CELL_CONTENTS(room, x, z) (&room->cellContents[(x) * room->spanZ + (z)])

int collisionObjectRoomColliders(struct Room* room, struct Box3D* box, short output[MAX_COLLIDERS]) {
    short tmp[MAX_COLLIDERS];

    short* currentSource = tmp;
    short* currentResult = output;
    int result = 0;

    int minX = GRID_CELL_X(room, box->min.x);
    int maxX = GRID_CELL_X(room, box->max.x);

    int minZ = GRID_CELL_Z(room, box->min.z);
    int maxZ = GRID_CELL_Z(room, box->max.z);
    
    for (int x = MAX(minX, 0); x <= maxX && x < room->spanX; ++x) {
        for (int z = MAX(minZ, 0); z <= maxZ && z < room->spanZ; ++z) {
            struct Rangeu16* range = GRID_CELL_CONTENTS(room, x, z);

            result = mergeColliderList(currentSource, result, &room->quadIndices[range->min], range->max - range->min, currentResult);

            if (currentResult == output) {
                currentResult = tmp;
                currentSource = output;
            } else {
                currentResult = output;
                currentSource = tmp;
            }
        }
    }

    if (currentSource != output) {
        mergeColliderList(currentSource, result, NULL, 0, output);
    }

    return result;
}

void collisionObjectCollideMixed(struct CollisionObject* object, struct Vector3* objectPrevPos, struct Box3D* sweptBB, struct CollisionScene* scene, struct ContactSolver* contactSolver) {    
    short colliderIndices[MAX_COLLIDERS];
    int quadCount = collisionObjectRoomColliders(&scene->world->rooms[object->body->currentRoom], sweptBB, colliderIndices);

    for (int i = 0; i < quadCount; ++i) {
        int quadIndex = colliderIndices[i];
        struct CollisionObject* quad = &scene->quads[quadIndex];
        int shouldCheckPortals = gCollisionScene.portalColliderIndex[0] == quadIndex || gCollisionScene.portalColliderIndex[1] == quadIndex;

        if (quad->manifoldIds & object->manifoldIds) {
            collisionObjectCollideWithQuad(object, quad, contactSolver, shouldCheckPortals);
        } else {
            collisionObjectCollideWithQuadSwept(object, objectPrevPos, sweptBB, quad, contactSolver, shouldCheckPortals);
        }
    }
}


int collisionSceneFilterPortalContacts(struct ContactManifold* contact) {
    int writeIndex = 0;

    for (int readIndex = 0; readIndex < contact->contactCount; ++readIndex) {
        if (collisionSceneIsTouchingPortal(&contact->contacts[readIndex].contactALocal, &contact->normal)) {
            continue;
        }

        if (readIndex != writeIndex) {
            contact->contacts[writeIndex] = contact->contacts[readIndex];
        }

        ++writeIndex;
    }

    contact->contactCount = writeIndex;

    return writeIndex;
}

int collisionSceneIsTouchingSinglePortal(struct Vector3* contactPoint, struct Vector3* contactNormal, struct Transform* portalTransform, int portalIndex) {
    struct Vector3 localPoint;
    transformPointInverseNoScale(portalTransform, contactPoint, &localPoint);

    if (fabsf(localPoint.z) > PORTAL_THICKNESS) {
        return 0;
    }

    localPoint.x *= (1.0f / PORTAL_X_RADIUS);
    localPoint.z = 0.0f;

    if (vector3MagSqrd(&localPoint) >= 1.0f) {
        return 0;
    }

    struct Vector3 portalNormal = gZeroVec;
    portalNormal.z = portalIndex ? 1.0f : -1.0f;
    quatMultVector(&portalTransform->rotation, &portalNormal, &portalNormal);

    return vector3Dot(contactNormal, &portalNormal) > 0.0f;
}

int collisionSceneIsTouchingPortal(struct Vector3* contactPoint, struct Vector3* contactNormal) {
    if (!collisionSceneIsPortalOpen()) {
        return 0;
    }

    for (int i = 0; i < 2; ++i) {
        if (collisionSceneIsTouchingSinglePortal(contactPoint, contactNormal, gCollisionScene.portalTransforms[i], i)) {
            return RigidBodyIsTouchingPortalA << i;
        }
    }

    return 0;
}

int collisionSceneIsPortalOpen() {
    return gCollisionScene.portalTransforms[0] != NULL && gCollisionScene.portalTransforms[1] != NULL;
}

void collisionSceneSetPortal(int portalIndex, struct Transform* transform, int roomIndex, int colliderIndex) {
    gCollisionScene.portalTransforms[portalIndex] = transform;
    gCollisionScene.portalRooms[portalIndex] = roomIndex;
    gCollisionScene.portalColliderIndex[portalIndex] = colliderIndex;

    if (gCollisionScene.portalTransforms[1 - portalIndex]) {
        struct Transform inverseTransform;
        transformInvert(transform, &inverseTransform);
        transformConcat(gCollisionScene.portalTransforms[1 - portalIndex], &inverseTransform, &gCollisionScene.toOtherPortalTransform[portalIndex]);
        transformInvert(&gCollisionScene.toOtherPortalTransform[portalIndex], &gCollisionScene.toOtherPortalTransform[1 - portalIndex]);
    }
}

struct Transform* collisionSceneTransformToPortal(int fromPortal) {
    if (!collisionSceneIsPortalOpen()) {
        return NULL;
    }

    return &gCollisionScene.toOtherPortalTransform[fromPortal];
}

void collisionScenePushObjectsOutOfPortal(int portalIndex) {
    if (!gCollisionScene.portalTransforms[portalIndex]) {
        return;
    }

    struct Transform* portalTransform = gCollisionScene.portalTransforms[portalIndex];

    struct Vector3 reversePortalNormal = gZeroVec;
    reversePortalNormal.z = portalIndex ? -1.0f : 1.0f;
    quatMultVector(&portalTransform->rotation, &reversePortalNormal, &reversePortalNormal);
    
    for (unsigned i = 0; i < gCollisionScene.dynamicObjectCount; ++i) {
        struct CollisionObject* object = gCollisionScene.dynamicObjects[i];

        if (!(object->body->flags & ((RigidBodyIsTouchingPortalA | RigidBodyWasTouchingPortalA) << portalIndex))) {
            continue;
        }

        struct Vector3 colliderPoint;
        minkowsiSumAgainstObject(object, &reversePortalNormal, &colliderPoint);

        struct Vector3 offset;
        vector3Sub(&portalTransform->position, &colliderPoint, &offset);

        float depth = vector3Dot(&offset, &reversePortalNormal);

        if (depth > 0.0f) {
            continue;
        }

        vector3AddScaled(&object->body->transform.position, &reversePortalNormal, depth, &object->body->transform.position);
    }
}

void collisionSceneRaycastRoom(struct CollisionScene* scene, struct Room* room, struct Ray* ray, int collisionLayers, struct RaycastHit* hit) {
    int currX = GRID_CELL_X(room, ray->origin.x);
    int currZ = GRID_CELL_Z(room, ray->origin.z);

    float xDirInv = fabsf(ray->dir.x) > 0.00001f ? 1.0f / ray->dir.x : 0.0f;
    float zDirInv = fabsf(ray->dir.z) > 0.00001f ? 1.0f / ray->dir.z : 0.0f;

    if ((currX < 0 || currX >= room->spanX) && xDirInv != 0.0f) {
        int boundX = (ray->dir.x > 0.0f ? 0 : room->spanX) * COLLISION_GRID_CELL_SIZE + room->cornerX;

        float distanceToEdge = (boundX - ray->origin.x) * xDirInv;

        if (distanceToEdge > 0.0f) {
            float zAtDistance = ray->dir.z * distanceToEdge + ray->origin.z;

            int zCheck = GRID_CELL_Z(room, zAtDistance);

            if (zCheck >= 0 && zCheck < room->spanZ) {
                currX = ray->dir.x > 0.0f ? 0 : room->spanX - 1;
                currZ = zCheck;
            }
        }
    }

    if ((currZ < 0 || currZ >= room->spanZ) && zDirInv != 0.0f) {
        int boundZ = (ray->dir.z > 0.0f ? 0 : room->spanZ) * COLLISION_GRID_CELL_SIZE + room->cornerZ;

        float distanceToEdge = (boundZ - ray->origin.z) * zDirInv;

        if (distanceToEdge > 0.0f) {
            float xAtDistance = ray->dir.x * distanceToEdge + ray->origin.x;
            int xCheck = GRID_CELL_X(room, xAtDistance);

            if (xCheck >= 0 && xCheck < room->spanX) {
                currX = xCheck;
                currZ = ray->dir.z > 0.0f ? 0 : room->spanZ - 1;
            }
        }
    }

    while (currX >=0 && currX < room->spanX && currZ >= 0 && currZ < room->spanZ) {
        struct Rangeu16* range = GRID_CELL_CONTENTS(room, currX, currZ);

        for (int i = range->min; i < range->max; ++i) {
            struct RaycastHit hitTest;

            struct CollisionObject* collisionObject = &scene->quads[room->quadIndices[i]];

            if ((collisionObject->collisionLayers & collisionLayers) == 0) {
                continue;
            }

            if (raycastQuad(collisionObject, ray, hit->distance, &hitTest) && hitTest.distance < hit->distance && vector3Dot(&hitTest.normal, &ray->dir) < 0.0f) {
                hit->at = hitTest.at;
                hit->normal = hitTest.normal;
                hit->distance = hitTest.distance;
                hit->object = hitTest.object;
            }
        }

        int xStep = 0;
        int zStep = 0;
        float cellDistance = hit->distance;

        if (xDirInv != 0.0f) {
            float nextEdge = (currX + (ray->dir.x > 0.0f ? 1 : 0)) * COLLISION_GRID_CELL_SIZE + room->cornerX;
            float distanceCheck = (nextEdge - ray->origin.x) * xDirInv;

            if (distanceCheck < cellDistance) {
                cellDistance = distanceCheck;
                xStep = ray->dir.x > 0.0f ? 1 : -1;
                zStep = 0;
            }
        }

        if (zDirInv != 0.0f) {
            float nextEdge = (currZ + (ray->dir.z > 0.0f ? 1 : 0)) * COLLISION_GRID_CELL_SIZE + room->cornerZ;
            float distanceCheck = (nextEdge - ray->origin.z) * zDirInv;

            if (distanceCheck < cellDistance) {
                cellDistance = distanceCheck;
                xStep = 0;
                zStep = ray->dir.z > 0.0f ? 1 : -1;
            }
        }

        if (!xStep && !zStep) {
            return;
        }

        currX += xStep;
        currZ += zStep;
    }
}

int collisionSceneRaycastDoorways(struct CollisionScene* scene, struct Room* room, struct Ray* ray, float maxDistance, int currentRoom) {
    int nextRoom = -1;

    float roomDistance = maxDistance;

    for (int i = 0; i < room->doorwayCount; ++i) {
        struct RaycastHit hitTest;

        struct Doorway* doorway = &scene->world->doorways[room->doorwayIndices[i]];

        if ((doorway->flags & DoorwayFlagsOpen) == 0) {
            continue;
        }

        if (raycastQuadShape(&doorway->quad, ray, roomDistance, &hitTest) && hitTest.distance < roomDistance) {
            roomDistance = hitTest.distance;
            nextRoom = currentRoom == doorway->roomA ? doorway->roomB : doorway->roomA;
        }
    }

    return nextRoom;
}

int collisionSceneRaycast(struct CollisionScene* scene, int roomIndex, struct Ray* ray, int collisionLayers, float maxDistance, int passThroughPortals, struct RaycastHit* hit) {
    hit->distance = maxDistance;
    hit->throughPortal = NULL;

    int roomsToCheck = 5;

    while (roomsToCheck && roomIndex != -1) {
        struct Room* room = &scene->world->rooms[roomIndex];
        collisionSceneRaycastRoom(scene, room, ray, collisionLayers, hit);

        if (hit->distance != maxDistance) {
            hit->roomIndex = roomIndex;
            break;
        }

        int nextRoom = collisionSceneRaycastDoorways(scene, room, ray, hit->distance, roomIndex);

        roomIndex = nextRoom;

        --roomsToCheck;
    }

    for (int i = 0; i < scene->dynamicObjectCount; ++i) {
        struct RaycastHit hitTest;

        struct CollisionObject* object = scene->dynamicObjects[i];

        if ((object->collisionLayers & collisionLayers) == 0) {
            continue;
        }

        if (object->collider->callbacks->raycast && 
            object->collider->callbacks->raycast(object, ray, hit->distance, &hitTest) &&
            hitTest.distance < hit->distance) {
            hit->at = hitTest.at;
            hit->normal = hitTest.normal;
            hit->distance = hitTest.distance;
            hit->object = hitTest.object;
            hit->roomIndex = hitTest.roomIndex;
        }
    }

    if (passThroughPortals && 
        hit->distance != maxDistance &&
        collisionSceneIsPortalOpen()) {
        for (int i = 0; i < 2; ++i) {
            if (collisionSceneIsTouchingSinglePortal(&hit->at, &hit->normal, gCollisionScene.portalTransforms[i], i)) {
                struct Transform portalTransform;
                collisionSceneGetPortalTransform(i, &portalTransform);

                struct Ray newRay;

                transformPoint(&portalTransform, &hit->at, &newRay.origin);
                quatMultVector(&portalTransform.rotation, &ray->dir, &newRay.dir);

                struct RaycastHit newHit;

                int result = collisionSceneRaycast(scene, gCollisionScene.portalRooms[1 - i], &newRay, collisionLayers, maxDistance - hit->distance, 0, &newHit);

                if (result) {
                    newHit.distance += hit->distance;
                    newHit.throughPortal = gCollisionScene.portalTransforms[i];
                    *hit = newHit;
                }

                return result;
            }
        }
    }

    return hit->distance != maxDistance;
}

void collisionSceneGetPortalTransform(int fromPortal, struct Transform* out) {
    struct Transform inverseA;
    transformInvert(gCollisionScene.portalTransforms[fromPortal], &inverseA);
    transformConcat(gCollisionScene.portalTransforms[1 - fromPortal], &inverseA, out);
}

void collisionSceneAddDynamicObject(struct CollisionObject* object) {
    if (gCollisionScene.dynamicObjectCount < MAX_DYNAMIC_OBJECTS) {
        gCollisionScene.dynamicObjects[gCollisionScene.dynamicObjectCount] = object;
        ++gCollisionScene.dynamicObjectCount;
    }
}

void collisionSceneRemoveDynamicObject(struct CollisionObject* object) {
    int found = 0;

    for (unsigned i = 0; i < gCollisionScene.dynamicObjectCount; ++i) {
        if (object == gCollisionScene.dynamicObjects[i]) {
            found = 1;
        }

        if (found && i + 1 < gCollisionScene.dynamicObjectCount) {
            gCollisionScene.dynamicObjects[i] = gCollisionScene.dynamicObjects[i + 1];
        }
    }

    if (found) {
        --gCollisionScene.dynamicObjectCount;
    }
}

#define BROADPHASE_SCALE    16.0f

union DynamicBroadphaseEdge {
    struct {
        unsigned short isLeadingEdge: 1;
        unsigned short objectId: 7;
        short sortKey;
    };
    int align;
};

struct DynamicBroadphase {
    union DynamicBroadphaseEdge* edges;
    short* objectsInCurrentRange;
    int objectInRangeCount;
};

void dynamicBroadphasePopulate(struct DynamicBroadphase* broadphase, struct CollisionObject** objects, int count) {
    int edgeIndex = 0;

    for (int i = 0; i < count; ++i) {
        union DynamicBroadphaseEdge edge;
        edge.isLeadingEdge = 1;
        edge.objectId = i;
        edge.sortKey = (short)floorf(objects[i]->boundingBox.min.x * BROADPHASE_SCALE);

        broadphase->edges[edgeIndex++] = edge;

        edge.isLeadingEdge = 0;
        edge.sortKey = (short)ceilf(objects[i]->boundingBox.max.x * BROADPHASE_SCALE);

        broadphase->edges[edgeIndex++] = edge;
    }
}

void dynamicBroadphaseSort(union DynamicBroadphaseEdge* edges, union DynamicBroadphaseEdge* tmp, int min, int max) {
    if (min + 1 >= max) {
        return;
    }

    int middle = (min + max) >> 1;
    dynamicBroadphaseSort(edges, tmp, min, middle);
    dynamicBroadphaseSort(edges, tmp, middle, max);

    int aHead = min;
    int bHead = middle;
    int output = min;

    while (aHead < middle && bHead < max) {
        int sortDifference = (int)edges[aHead].sortKey - (int)edges[bHead].sortKey;

        if (sortDifference <= 0) {
            tmp[output].align = edges[aHead].align;
            ++output;
            ++aHead;
        } else {
            tmp[output].align = edges[bHead].align;
            ++output;
            ++bHead;
        }
    }

    while (aHead < middle) {
        tmp[output].align = edges[aHead].align;
        ++output;
        ++aHead;
    }

    while (bHead < max) {
        tmp[output].align = edges[bHead].align;
        ++output;
        ++bHead;
    }

    for (output = min; output < max; ++output) {
        edges[output] = tmp[output];
    }
}

void collisionObjectCollidePairMixed(struct CollisionObject* a, struct Vector3* aPrevPos, struct Box3D* sweptA, struct CollisionObject* b, struct Vector3* bPrevPos, struct Box3D* sweptB, struct ContactSolver* contactSolver) {
    if (a->manifoldIds & b->manifoldIds) {
        collisionObjectCollideTwoObjects(a, b, contactSolver);
    } else {
        collisionObjectCollideTwoObjectsSwept(a, aPrevPos, sweptA, b, bPrevPos, sweptB, contactSolver);
    }
}

void collisionSceneWalkBroadphase(struct CollisionScene* collisionScene, struct DynamicBroadphase* broadphase, struct Vector3* prevPos, struct Box3D* sweptBB) {
    int broadphaseEdgeCount = collisionScene->dynamicObjectCount * 2;
    for (int i = 0; i < broadphaseEdgeCount; ++i) {
        union DynamicBroadphaseEdge edge;
        edge.align = broadphase->edges[i].align;

        struct CollisionObject* subject = collisionScene->dynamicObjects[edge.objectId];

        if (edge.isLeadingEdge) {
            for (int objectIndex = 0; objectIndex < broadphase->objectInRangeCount; ++objectIndex) {
                short existingIndex = broadphase->objectsInCurrentRange[objectIndex];
                struct CollisionObject* existing = collisionScene->dynamicObjects[existingIndex];

                if ((existing->collisionLayers & subject->collisionLayers) == 0) {
                    continue;
                }

                if (!collisionObjectShouldGenerateConctacts(existing) && !collisionObjectShouldGenerateConctacts(subject)) {
                    continue;
                }

                // collide pair lowest in memory first
                if (existing < subject) {
                    collisionObjectCollidePairMixed(
                        existing, 
                        &prevPos[existingIndex], 
                        &sweptBB[existingIndex], 
                        subject, 
                        &prevPos[edge.objectId], 
                        &sweptBB[edge.objectId], 
                        &gContactSolver
                    );
                } else {
                    collisionObjectCollidePairMixed(
                        subject, 
                        &prevPos[edge.objectId],
                        &sweptBB[edge.objectId], 
                        existing, 
                        &prevPos[existingIndex], 
                        &sweptBB[existingIndex],
                        &gContactSolver
                    );
                }
            }

            // add object
            broadphase->objectsInCurrentRange[broadphase->objectInRangeCount] = edge.objectId;
            ++broadphase->objectInRangeCount;
        } else {
            // remove object
            int hasFound = 0;
            for (int i = 0; i < broadphase->objectInRangeCount - 1; ++i) {
                if (broadphase->objectsInCurrentRange[i] == edge.objectId) {
                    hasFound = 1;
                }

                if (hasFound) {
                    broadphase->objectsInCurrentRange[i] = broadphase->objectsInCurrentRange[i + 1];
                }
            }

            --broadphase->objectInRangeCount;
        }
    }
}

void collisionSceneUpdateObjectCurrentRooms(struct Vector3* prevPosList){
    for (unsigned i = 0; i < gCollisionScene.dynamicObjectCount; ++i) {
        int doorwayMask = worldCheckDoorwaySides(&gCurrentLevel->world, &prevPosList[i], gCollisionScene.dynamicObjects[i]->body->currentRoom);
        gCollisionScene.dynamicObjects[i]->body->currentRoom = worldCheckDoorwayCrossings(&gCurrentLevel->world, &gCollisionScene.dynamicObjects[i]->body->transform.position, gCollisionScene.dynamicObjects[i]->body->currentRoom, doorwayMask);
    }
}

void collisionSceneCollideDynamicPairs(struct CollisionScene* collisionScene, struct Vector3* prevPos, struct Box3D* sweptBB) {

    struct DynamicBroadphase dynamicBroadphase;

    dynamicBroadphase.edges = stackMalloc(sizeof(union DynamicBroadphaseEdge) * collisionScene->dynamicObjectCount * 2);
    dynamicBroadphasePopulate(&dynamicBroadphase, collisionScene->dynamicObjects, collisionScene->dynamicObjectCount);

    union DynamicBroadphaseEdge* tmpEdges = stackMalloc(sizeof(union DynamicBroadphaseEdge) * collisionScene->dynamicObjectCount * 2);
    dynamicBroadphaseSort(dynamicBroadphase.edges, tmpEdges, 0, collisionScene->dynamicObjectCount * 2);
    stackMallocFree(tmpEdges);

    dynamicBroadphase.objectsInCurrentRange = stackMalloc(sizeof(short) * collisionScene->dynamicObjectCount);
    dynamicBroadphase.objectInRangeCount = 0;

    collisionSceneWalkBroadphase(collisionScene, &dynamicBroadphase, prevPos, sweptBB);

    collisionSceneUpdateObjectCurrentRooms(prevPos);

    stackMallocFree(dynamicBroadphase.objectsInCurrentRange);
    stackMallocFree(dynamicBroadphase.edges);
}

int collisionSceneObjectIsTouchingPortal(struct CollisionObject* object, int portalIndex) {
    struct Simplex simplex;
    struct Vector3 direction;
    quatMultVector(&gCollisionScene.portalTransforms[portalIndex]->rotation, &gRight, &direction);
    return gjkCheckForOverlap(&simplex, 
        object, minkowsiSumAgainstObject,
        gCollisionScene.portalTransforms[portalIndex], minkowsiSumAgainstPortal,
        &direction
    );   
}

void collisionSceneUpdateDynamics() {
    for (unsigned i = 0; i < gCollisionScene.dynamicObjectCount; ++i) {
        // added back in by contactSolverRemoveUnusedContacts if there are actually contacts
        gCollisionScene.dynamicObjects[i]->flags &= ~COLLISION_OBJECT_HAS_CONTACTS;
    }

	contactSolverRemoveUnusedContacts(&gContactSolver);

    struct Vector3* prevPosList = stackMalloc(sizeof(struct Vector3) * gCollisionScene.dynamicObjectCount);
    struct Box3D* sweptBB = stackMalloc(sizeof(struct Box3D) * gCollisionScene.dynamicObjectCount);

    for (unsigned i = 0; i < gCollisionScene.dynamicObjectCount; ++i) {
        struct CollisionObject* object = gCollisionScene.dynamicObjects[i];

        prevPosList[i] = object->body->transform.position;
        sweptBB[i] = object->boundingBox;

        if (!collisionObjectShouldGenerateConctacts(object)) {
            continue;
        }

        if (object->body->flags & (RigidBodyIsTouchingPortalA | RigidBodyWasTouchingPortalA)) {
            if (collisionSceneObjectIsTouchingPortal(object, 0)) {
                object->body->flags |= RigidBodyIsTouchingPortalA;
            }
        }

        if (object->body->flags & (RigidBodyIsTouchingPortalB | RigidBodyWasTouchingPortalB)) {
            if (collisionSceneObjectIsTouchingPortal(object, 1)) {
                object->body->flags |= RigidBodyIsTouchingPortalB;
            }
        }

        if (!collisionObjectIsActive(object)) {
            continue;
        }

        rigidBodyUpdate(object->body);
        collisionObjectUpdateBB(object);
        box3DUnion(&sweptBB[i], &object->boundingBox, &sweptBB[i]);

        collisionObjectCollideMixed(object, &prevPosList[i], &sweptBB[i], &gCollisionScene, &gContactSolver);
    }

    collisionSceneCollideDynamicPairs(&gCollisionScene, prevPosList, sweptBB);

    stackMallocFree(sweptBB);
    stackMallocFree(prevPosList);

    contactSolverSolve(&gContactSolver);

    for (unsigned i = 0; i < gCollisionScene.dynamicObjectCount; ++i) {
        struct CollisionObject* collisionObject = gCollisionScene.dynamicObjects[i];
        if (!collisionObjectIsActive(collisionObject)) {
            // kind of a hack, but the player is the only kinematic body that
            // also generates contacts and the player velocty should not be 
            // cleared
            if (!collisionObjectShouldGenerateConctacts(collisionObject)) {
                // clear out any velocities
                collisionObject->body->velocity = gZeroVec;
                collisionObject->body->angularVelocity = gZeroVec;
            }
            continue;
        }

        rigidBodyCheckPortals(collisionObject->body);
        collisionObjectUpdateBB(collisionObject);
    }
}
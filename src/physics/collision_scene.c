#include "collision_scene.h"

#include "math/mathf.h"

struct CollisionScene gCollisionScene;

void collisionSceneInit(struct CollisionScene* scene, struct CollisionObject* quads, int quadCount) {
    scene->quads = quads;
    scene->quadCount = quadCount;

    scene->dynamicObjectCount = 0;
    scene->portalTransforms[0] = NULL;
    scene->portalTransforms[1] = NULL;
}

void collisionObjectCollideWithScene(struct CollisionObject* object, struct CollisionScene* scene, struct ContactSolver* contactSolver) {
    for (int i = 0; i < scene->quadCount; ++i) {
        collisionObjectCollideWithQuad(object, &scene->quads[i], contactSolver);
    }
}

int collisionSceneFilterPortalContacts(struct ContactConstraintState* contact) {
    int writeIndex = 0;

    for (int readIndex = 0; readIndex < contact->contactCount; ++readIndex) {
        if (collisionSceneIsTouchingPortal(&contact->contacts[readIndex].ra)) {
            continue;;
        }

        if (readIndex != writeIndex) {
            contact->contacts[writeIndex] = contact->contacts[readIndex];
        }

        ++writeIndex;
    }

    contact->contactCount = writeIndex;

    return writeIndex;
}

void collisionObjectQueryScene(struct CollisionObject* object, struct CollisionScene* scene, void* data, ManifoldCallback callback) {
    CollideWithQuad quadCollider = object->collider->callbacks->collideWithQuad;

    if (!quadCollider) {
        return;
    }

    struct ContactConstraintState localContact;

    for (int i = 0; i < scene->quadCount; ++i) {
        localContact.contactCount = 0;

        if (quadCollider(object->collider->data, &object->body->transform, scene->quads[i].collider->data, &localContact) &&
            collisionSceneFilterPortalContacts(&localContact)) {
            callback(data, &localContact);
        }
    }
}

int collisionSceneIsTouchingSinglePortal(struct Vector3* contactPoint, struct Transform* portalTransform) {
    struct Vector3 localPoint;
    transformPointInverse(portalTransform, contactPoint, &localPoint);

    if (fabsf(localPoint.z) > PORTAL_THICKNESS) {
        return 0;
    }

    localPoint.x *= (1.0f / PORTAL_X_RADIUS);
    localPoint.z = 0.0f;

    return vector3MagSqrd(&localPoint) < 1.0f;
}

int collisionSceneIsTouchingPortal(struct Vector3* contactPoint) {
    if (!collisionSceneIsPortalOpen()) {
        return 0;
    }

    for (int i = 0; i < 2; ++i) {
        if (collisionSceneIsTouchingSinglePortal(contactPoint, gCollisionScene.portalTransforms[i])) {
            return 1;
        }
    }

    return 0;
}

int collisionSceneIsPortalOpen() {
    return gCollisionScene.portalTransforms[0] != NULL && gCollisionScene.portalTransforms[1] != NULL;
}

int collisionSceneRaycast(struct CollisionScene* scene, struct Ray* ray, float maxDistance, int passThroughPortals, struct RaycastHit* hit) {
    hit->distance = maxDistance;
    hit->throughPortal = NULL;
    
    for (int i = 0; i < scene->quadCount; ++i) {
        struct RaycastHit hitTest;

        if (raycastQuad(&scene->quads[i], ray, hit->distance, &hitTest) && hitTest.distance < hit->distance) {
            hit->at = hitTest.at;
            hit->normal = hitTest.normal;
            hit->distance = hitTest.distance;
            hit->object = hitTest.object;
        }
    }

    for (int i = 0; i < scene->dynamicObjectCount; ++i) {
        struct RaycastHit hitTest;

        struct CollisionObject* object = scene->dynamicObjects[i];

        if (object->collider->callbacks->raycast && 
            object->collider->callbacks->raycast(object, ray, hit->distance, &hitTest) &&
            hitTest.distance < hit->distance) {
            hit->at = hitTest.at;
            hit->normal = hitTest.normal;
            hit->distance = hitTest.distance;
            hit->object = hitTest.object;
        }
    }

    if (passThroughPortals && 
        hit->distance != maxDistance &&
        collisionSceneIsPortalOpen()) {
        for (int i = 0; i < 2; ++i) {
            if (collisionSceneIsTouchingSinglePortal(&hit->at, gCollisionScene.portalTransforms[i])) {
                struct Transform portalTransform;
                collisionSceneGetPortalTransform(i, &portalTransform);

                struct Ray newRay;

                transformPoint(&portalTransform, &hit->at, &newRay.origin);
                quatMultVector(&portalTransform.rotation, &ray->dir, &newRay.dir);

                struct RaycastHit newHit;

                int result = collisionSceneRaycast(scene, &newRay, maxDistance - hit->distance, 0, &newHit);

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

void collisionSceneUpdateDynamics() {
    for (unsigned i = 0; i < gCollisionScene.dynamicObjectCount; ++i) {
        collisionObjectCollideWithScene(gCollisionScene.dynamicObjects[i], &gCollisionScene, &gContactSolver);
    }

    contactSolverSolve(&gContactSolver);

    for (unsigned i = 0; i < gCollisionScene.dynamicObjectCount; ++i) {
        rigidBodyUpdate(gCollisionScene.dynamicObjects[i]->body);
        rigidBodyCheckPortals(gCollisionScene.dynamicObjects[i]->body);
    }
}
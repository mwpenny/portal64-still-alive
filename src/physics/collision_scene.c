#include "collision_scene.h"

#include "math/mathf.h"

struct CollisionScene gCollisionScene;

void collisionSceneInit(struct CollisionScene* scene, struct CollisionObject* quads, int quadCount) {
    scene->quads = quads;
    scene->quadCount = quadCount;
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

int collisionSceneIsTouchingPortal(struct Vector3* contactPoint) {
    if (!collisionSceneIsPortalOpen()) {
        return 0;
    }

    for (int i = 0; i < 2; ++i) {
        struct Vector3 localPoint;
        transformPointInverse(gCollisionScene.portalTransforms[i], contactPoint, &localPoint);

        if (fabsf(localPoint.z) > PORTAL_THICKNESS) {
            continue;
        }

        localPoint.x *= (1.0f / PORTAL_X_RADIUS);
        localPoint.z = 0.0f;

        if (vector3MagSqrd(&localPoint) < 1.0f) {
            return 1;
        }
    }

    return 0;
}

int collisionSceneIsPortalOpen() {
    return gCollisionScene.portalTransforms[0] != NULL && gCollisionScene.portalTransforms[1] != NULL;
}
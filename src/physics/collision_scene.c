#include "collision_scene.h"

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

void collisionObjectQueryScene(struct CollisionObject* object, struct CollisionScene* scene, void* data, ManifoldCallback callback) {
    CollideWithQuad quadCollider = object->collider->callbacks->collideWithQuad;

    if (!quadCollider) {
        return;
    }

    struct ContactConstraintState localContact;

    for (int i = 0; i < scene->quadCount; ++i) {
        localContact.contactCount = 0;

        if (quadCollider(object->collider->data, &object->body->transform, scene->quads[i].collider->data, &localContact)) {
            callback(data, &localContact);
        }
    }
}
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
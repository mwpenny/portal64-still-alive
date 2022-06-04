#include "button.h"

#include "defs.h"
#include "../models/models.h"
#include "../graphics/renderstate.h"
#include "dynamic_scene.h"
#include "../physics/collision_cylinder.h"
#include "../physics/collision_scene.h"
#include "../physics/contact_solver.h"
#include "../util/time.h"

#include "../build/assets/models/props/button.h"

struct Vector2 gButtonCylinderEdgeVectors[] = {
    {0.0f, 1.0f},
    {0.707f, 0.707f},
    {1.0f, 0.0f},
    {0.707f, -0.707f},
};

struct CollisionCylinder gButtonCylinder = {
    0.5f,
    0.3f,
    gButtonCylinderEdgeVectors,
    sizeof(gButtonCylinderEdgeVectors) / sizeof(*gButtonCylinderEdgeVectors),
};

struct ColliderTypeData gButtonCollider = {
    CollisionShapeTypeCylinder,
    &gButtonCylinder,
    0.0f,
    1.0f,
    &gCollisionCylinderCallbacks
};

#define MASS_BUTTON_PRESS_THRESHOLD     1.9f
#define BUTTON_MOVEMENT_AMOUNT          0.1f
#define BUTTON_MOVE_VELOCTY             0.2f

void buttonRender(void* data, struct RenderScene* renderScene) {
    struct Button* button = (struct Button*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);

    guTranslate(matrix, button->originalPos.x * SCENE_SCALE, button->originalPos.y * SCENE_SCALE, button->originalPos.z * SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderScene->renderState, PROPS_BUTTON_DEFAULT_BONES_COUNT);
    transformToMatrixL(&props_button_default_bones[PROPS_BUTTON_BUTTONBASE_BONE], &armature[PROPS_BUTTON_BUTTONBASE_BONE], 1.0f);

    // reusing global memory
    props_button_default_bones[PROPS_BUTTON_BUTTONPAD_BONE].position.y = (button->rigidBody.transform.position.y - button->originalPos.y) * SCENE_SCALE;
    transformToMatrixL(&props_button_default_bones[PROPS_BUTTON_BUTTONPAD_BONE], &armature[PROPS_BUTTON_BUTTONPAD_BONE], 1.0f);

    renderSceneAdd(renderScene, button_gfx, matrix, button_material_index, &button->rigidBody.transform.position, armature);
}

void buttonInit(struct Button* button, struct Vector3* at, int roomIndex) {
    collisionObjectInit(&button->collisionObject, &gButtonCollider, &button->rigidBody, 1.0f);
    rigitBodyMarkKinematic(&button->rigidBody);
    collisionSceneAddDynamicObject(&button->collisionObject);

    button->rigidBody.transform.position = *at;
    quatIdent(&button->rigidBody.transform.rotation);
    button->rigidBody.transform.scale = gOneVec;
    button->rigidBody.currentRoom = roomIndex;

    collisionObjectUpdateBB(&button->collisionObject);

    button->dynamicId = dynamicSceneAdd(button, buttonRender, &button->rigidBody.transform, 0.84f);
    button->pressStateCounter = 0;

    button->originalPos = *at;
}

void buttonUpdate(struct Button* button) {
    struct ContactManifold* manifold = contactSolverNextManifold(&gContactSolver, &button->collisionObject, NULL);

    int shouldPress = 0;

    while (manifold) {
        struct CollisionObject* other = manifold->shapeA == &button->collisionObject ? manifold->shapeB : manifold->shapeA;

        if (other->body && other->body->mass > MASS_BUTTON_PRESS_THRESHOLD) {
            shouldPress = 1;
            break;
        }

        manifold = contactSolverNextManifold(&gContactSolver, &button->collisionObject, manifold);
    }

    struct Vector3 targetPos = button->originalPos;
    
    if (shouldPress) {
        targetPos.y -= BUTTON_MOVEMENT_AMOUNT;
    }

    if (targetPos.y != button->rigidBody.transform.position.y) {
        vector3MoveTowards(&button->rigidBody.transform.position, &targetPos, BUTTON_MOVE_VELOCTY * FIXED_DELTA_TIME, &button->rigidBody.transform.position);
        collisionObjectUpdateBB(&button->collisionObject);
    }
}
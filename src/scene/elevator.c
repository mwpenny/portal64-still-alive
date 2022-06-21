#include "elevator.h"
#include "../physics/collision_scene.h"
#include "../scene/dynamic_scene.h"
#include "../models/models.h"
#include "../physics/mesh_collider.h"

#include "../../build/assets/models/props/round_elevator_collision.h"

struct ColliderTypeData gElevatorColliderType = {
    CollisionShapeTypeMesh,
    &props_round_elevator_collision_collider,
    0.0f, 0.6f,
    &gMeshColliderCallbacks
};

void elevatorRender(void* data, struct RenderScene* renderScene) {
    struct Elevator* elevator = (struct Elevator*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&elevator->rigidBody.transform, matrix, SCENE_SCALE);

    renderSceneAdd(
        renderScene, 
        elevator_gfx, 
        matrix, 
        elevator_material_index, 
        &elevator->rigidBody.transform.position, 
        NULL
    );
}

void elevatorInit(struct Elevator* elevator) {
    collisionObjectInit(&elevator->collisionObject, &gElevatorColliderType, &elevator->rigidBody, 1.0f, COLLISION_LAYERS_STATIC | COLLISION_LAYERS_TANGIBLE);
    rigidBodyMarkKinematic(&elevator->rigidBody);
    collisionSceneAddDynamicObject(&elevator->collisionObject);

    elevator->rigidBody.transform.position.x = -10.0f;
    elevator->rigidBody.transform.position.z = 2.5f;

    collisionObjectUpdateBB(&elevator->collisionObject);

    elevator->dynamicId = dynamicSceneAdd(elevator, elevatorRender, &elevator->rigidBody.transform, 3.9f);
}

void elevatorUpdate(struct Elevator* elevator) {

}
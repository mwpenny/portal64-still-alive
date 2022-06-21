#include "elevator.h"
#include "../physics/collision_scene.h"
#include "../scene/dynamic_scene.h"
#include "../models/models.h"

void elevatorRender(void* data, struct RenderScene* renderScene) {
    struct Elevator* elevator = (struct Elevator*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&elevator->transform, matrix, SCENE_SCALE);

    renderSceneAdd(
        renderScene, 
        elevator_gfx, 
        matrix, 
        elevator_material_index, 
        &elevator->transform.position, 
        NULL
    );
}

void elevatorInit(struct Elevator* elevator) {
    transformInitIdentity(&elevator->transform);

    elevator->transform.position.x = -10.0f;
    elevator->transform.position.z = 2.5f;

    elevator->dynamicId = dynamicSceneAdd(elevator, elevatorRender, &elevator->transform, 3.9f);
}

void elevatorUpdate(struct Elevator* elevator) {

}
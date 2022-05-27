#include "button.h"

#include "defs.h"
#include "../models/models.h"
#include "../graphics/renderstate.h"
#include "dynamic_scene.h"

void buttonRender(void* data, struct RenderScene* renderScene) {
    struct Button* button = (struct Button*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&button->rigidBody.transform, matrix, SCENE_SCALE);

    renderSceneAdd(renderScene, button_gfx, matrix, button_material_index, &button->rigidBody.transform.position);
}

void buttonInit(struct Button* button, struct Vector3* at, int roomIndex) {
    button->rigidBody.transform.position = *at;
    quatIdent(&button->rigidBody.transform.rotation);
    button->rigidBody.transform.scale = gOneVec;

    button->rigidBody.currentRoom = roomIndex;

    button->dynamicId = dynamicSceneAdd(button, buttonRender, &button->rigidBody.transform, 0.84f);
}

void buttonUpdate(struct Button* button) {

}
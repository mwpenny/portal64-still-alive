#include "button.h"

#include "defs.h"
#include "../models/models.h"
#include "../graphics/renderstate.h"
#include "dynamic_scene.h"

void buttonRender(void* data, struct RenderState* renderState) {
    struct Button* button = (struct Button*)data;
    Mtx* matrix = renderStateRequestMatrices(renderState, 1);
    transformToMatrixL(&button->rigidBody.transform, matrix, SCENE_SCALE);

    gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    gSPDisplayList(renderState->dl++, button_gfx);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}

void buttonInit(struct Button* button, struct Vector3* at, int roomIndex) {
    button->rigidBody.transform.position = *at;
    quatIdent(&button->rigidBody.transform.rotation);
    button->rigidBody.transform.scale = gOneVec;

    button->rigidBody.currentRoom = roomIndex;

    button->dynamicId = dynamicSceneAdd(button, buttonRender, &button->rigidBody.transform, 0.74f, button_material_index);
}

void buttonUpdate(struct Button* button) {

}
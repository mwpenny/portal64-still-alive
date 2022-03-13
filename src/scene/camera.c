
#include "camera.h"
#include "math/transform.h"
#include "defs.h"

void cameraInit(struct Camera* camera, float fov, float near, float far) {
    transformInitIdentity(&camera->transform);
    camera->fov = fov;
    camera->nearPlane = near;
    camera->farPlane = far;
}

void cameraBuildViewMatrix(struct Camera* camera, float matrix[4][4]) {
    struct Transform cameraTransCopy = camera->transform;
    vector3Scale(&cameraTransCopy.position, &cameraTransCopy.position, SCENE_SCALE);
    struct Transform inverse;
    transformInvert(&cameraTransCopy, &inverse);
    transformToMatrix(&inverse, matrix, 1.0f);
}

void cameraBuildProjectionMatrix(struct Camera* camera, float matrix[4][4], u16* perspectiveNormalize, float aspectRatio) {
    float planeScalar = 1.0f;

    if (camera->transform.position.y > camera->farPlane * 0.5f) {
        planeScalar = 2.0f * camera->transform.position.y / camera->farPlane;
    }

    guPerspectiveF(matrix, perspectiveNormalize, camera->fov, aspectRatio, camera->nearPlane * planeScalar, camera->farPlane * planeScalar, 1.0f);
}

Mtx* cameraSetupMatrices(struct Camera* camera, struct RenderState* renderState, float aspectRatio, u16* perspNorm) {
    Mtx* viewProjMatrix = renderStateRequestMatrices(renderState, 2);
    
    if (!viewProjMatrix) {
        return NULL;
    }

    guMtxIdent(&viewProjMatrix[0]);
    gSPMatrix(renderState->dl++, osVirtualToPhysical(&viewProjMatrix[0]), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);

    float view[4][4];
    float persp[4][4];
    float combined[4][4];

    cameraBuildViewMatrix(camera, view);
    u16 perspectiveNormalize;
    cameraBuildProjectionMatrix(camera, persp, &perspectiveNormalize, aspectRatio);
    guMtxCatF(view, persp, combined);
    guMtxF2L(combined, &viewProjMatrix[1]);

    gSPMatrix(renderState->dl++, osVirtualToPhysical(&viewProjMatrix[1]), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(renderState->dl++, perspectiveNormalize);

    if (perspNorm) {
        *perspNorm = perspectiveNormalize;
    }

    return &viewProjMatrix[1];
}
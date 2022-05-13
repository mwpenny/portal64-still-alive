
#include "camera.h"
#include "math/transform.h"
#include "defs.h"
#include "../graphics/graphics.h"

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

void cameraExtractClippingPlane(float viewPersp[4][4], struct Plane* output, int axis, float direction) {
    output->normal.x = viewPersp[0][axis] * direction + viewPersp[0][3];
    output->normal.y = viewPersp[1][axis] * direction + viewPersp[1][3];
    output->normal.z = viewPersp[2][axis] * direction + viewPersp[2][3];
    output->d = viewPersp[3][axis] * direction + viewPersp[3][3];
}

Mtx* cameraSetupMatrices(struct Camera* camera, struct RenderState* renderState, float aspectRatio, u16* perspNorm, Vp* viewport, struct FrustrumCullingInformation* clippingInfo) {
    Mtx* viewProjMatrix = renderStateRequestMatrices(renderState, 2);
    
    if (!viewProjMatrix) {
        return NULL;
    }

    guMtxIdent(&viewProjMatrix[0]);
    gSPMatrix(renderState->dl++, osVirtualToPhysical(&viewProjMatrix[0]), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);

    float view[4][4];
    float persp[4][4];
    float combined[4][4];

    float scaleX = viewport->vp.vscale[0] * (1.0f / (SCREEN_WD << 1));
    float scaleY = viewport->vp.vscale[1] * (1.0f / (SCREEN_HT << 1));

    float centerX = ((float)viewport->vp.vtrans[0] - (SCREEN_WD << 1)) * (1.0f / (SCREEN_WD << 1));
    float centerY = ((SCREEN_HT << 1) - (float)viewport->vp.vtrans[1]) * (1.0f / (SCREEN_HT << 1));

    guOrthoF(combined, centerX - scaleX, centerX + scaleX, centerY - scaleY, centerY + scaleY, 1.0f, -1.0f, 1.0f);
    u16 perspectiveNormalize;
    cameraBuildProjectionMatrix(camera, view, &perspectiveNormalize, aspectRatio);
    guMtxCatF(view, combined, persp);

    cameraBuildViewMatrix(camera, view);
    guMtxCatF(view, persp, combined);
    guMtxF2L(combined, &viewProjMatrix[1]);

    if (clippingInfo) {
        cameraExtractClippingPlane(combined, &clippingInfo->clippingPlanes[0], 0, 1.0f);
        cameraExtractClippingPlane(combined, &clippingInfo->clippingPlanes[1], 0, -1.0f);
        cameraExtractClippingPlane(combined, &clippingInfo->clippingPlanes[2], 1, 1.0f);
        cameraExtractClippingPlane(combined, &clippingInfo->clippingPlanes[3], 1, -1.0f);
        cameraExtractClippingPlane(combined, &clippingInfo->clippingPlanes[4], 2, 1.0f);
    }

    gSPMatrix(renderState->dl++, osVirtualToPhysical(&viewProjMatrix[1]), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(renderState->dl++, perspectiveNormalize);

    if (perspNorm) {
        *perspNorm = perspectiveNormalize;
    }

    return &viewProjMatrix[1];
}
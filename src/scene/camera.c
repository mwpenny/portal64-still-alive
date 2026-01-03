
#include "camera.h"
#include "graphics/graphics.h"
#include "math/mathf.h"
#include "math/transform.h"
#include "system/screen.h"

void frustumFromQuad(struct Vector3* cameraPos, struct CollisionQuad* quad, struct FrustumCullingInformation* out) {
    float isInFront = planePointDistance(&quad->plane, cameraPos) > 0;

    // Bottom left, bottom right, top right, top left
    struct Vector3 corners[4];

    // Find corners as seen from camera's POV
    if (isInFront) {
        corners[0] = quad->corner;
        vector3AddScaled(&quad->corner, &quad->edgeB, quad->edgeBLength, &corners[1]);
    } else {
        corners[1] = quad->corner;
        vector3AddScaled(&quad->corner, &quad->edgeB, quad->edgeBLength, &corners[0]);
    }
    vector3AddScaled(&corners[0], &quad->edgeA, quad->edgeALength, &corners[3]);
    vector3AddScaled(&corners[1], &quad->edgeA, quad->edgeALength, &corners[2]);

    struct Vector3 corner1Dir, corner2Dir, planeNormal;
    vector3Sub(cameraPos, &corners[3], &corner1Dir);

    // Build planes
    for (int i = 0; i < 4; ++i) {
        if (i % 2) {
            vector3Sub(cameraPos, &corners[i], &corner1Dir);
            vector3Cross(&corner2Dir, &corner1Dir, &planeNormal);
        } else {
            vector3Sub(cameraPos, &corners[i], &corner2Dir);
            vector3Cross(&corner1Dir, &corner2Dir, &planeNormal);
        }
        vector3Normalize(&planeNormal, &planeNormal);
        planeInitWithNormalAndPoint(&out->clippingPlanes[i], &planeNormal, cameraPos);
        out->clippingPlanes[i].d *= SCENE_SCALE;
    }

    out->clippingPlanes[CLIPPING_PLANE_NEAR] = quad->plane;
    out->clippingPlanes[CLIPPING_PLANE_NEAR].d *= SCENE_SCALE;

    if (isInFront) {
        vector3Negate(&out->clippingPlanes[CLIPPING_PLANE_NEAR].normal, &out->clippingPlanes[CLIPPING_PLANE_NEAR].normal);
        out->clippingPlanes[CLIPPING_PLANE_NEAR].d *= -1;
    }

    out->usedClippingPlaneCount = 5;
    out->cameraPos = *cameraPos;
}

enum FrustumResult isOutsideFrustum(struct FrustumCullingInformation* frustum, struct BoundingBoxs16* boundingBox) {
    enum FrustumResult result = FrustumResultInside;

    for (int i = 0; i < frustum->usedClippingPlaneCount; ++i) {
        struct Vector3 closestPoint;

        struct Vector3* normal = &frustum->clippingPlanes[i].normal;

        closestPoint.x = normal->x < 0.0f ? boundingBox->minX : boundingBox->maxX;
        closestPoint.y = normal->y < 0.0f ? boundingBox->minY : boundingBox->maxY;
        closestPoint.z = normal->z < 0.0f ? boundingBox->minZ : boundingBox->maxZ;

        if (planePointDistance(&frustum->clippingPlanes[i], &closestPoint) < 0.00001f) {
            return FrustumResultOutisde;
        }

        if (result == FrustumResultBoth) {
            continue;
        }

        // now check if it is fully contained
        closestPoint.x = normal->x > 0.0f ? boundingBox->minX : boundingBox->maxX;
        closestPoint.y = normal->y > 0.0f ? boundingBox->minY : boundingBox->maxY;
        closestPoint.z = normal->z > 0.0f ? boundingBox->minZ : boundingBox->maxZ;
        
        if (planePointDistance(&frustum->clippingPlanes[i], &closestPoint) < 0.00001f) {
            result = FrustumResultBoth;
        }
    }

    return result;
}

int isRotatedBoxOutsideFrustum(struct FrustumCullingInformation* frustum, struct RotatedBox* rotatedBox) {
    for (int i = 0; i < frustum->usedClippingPlaneCount; ++i) {
        struct Vector3 closestPoint = rotatedBox->origin;

        struct Vector3* normal = &frustum->clippingPlanes[i].normal;

        for (int axis = 0; axis < 3; ++axis) {
            if (vector3Dot(&rotatedBox->sides[axis], normal) > 0.0f) {
                vector3Add(&closestPoint, &rotatedBox->sides[axis], &closestPoint);
            }
        }

        if (planePointDistance(&frustum->clippingPlanes[i], &closestPoint) < 0.00001f) {
            return 1;
        }
    }

    return 0;
}

int isSphereOutsideFrustum(struct FrustumCullingInformation* frustum, struct Vector3* scaledCenter, float scaledRadius) {
    for (int i = 0; i < frustum->usedClippingPlaneCount; ++i) {
        if (planePointDistance(&frustum->clippingPlanes[i], scaledCenter) < -scaledRadius) {
            return 1;
        }
    }

    return 0;
}

int isQuadOutsideFrustum(struct FrustumCullingInformation* frustum, struct CollisionQuad* quad) {
    for (int i = 0; i < frustum->usedClippingPlaneCount; ++i) {
        struct Vector3* normal = &frustum->clippingPlanes[i].normal;
        float aLerp = vector3Dot(normal, &quad->edgeA) < 0.0f ? 0.0f : quad->edgeALength;
        float bLerp = vector3Dot(normal, &quad->edgeB) < 0.0f ? 0.0f : quad->edgeBLength;

        struct Vector3 closestPoint;
        vector3AddScaled(&quad->corner, &quad->edgeA, aLerp, &closestPoint);
        vector3AddScaled(&closestPoint, &quad->edgeB, bLerp, &closestPoint);

        vector3Scale(&closestPoint, &closestPoint, SCENE_SCALE);

        if (planePointDistance(&frustum->clippingPlanes[i], &closestPoint) < 0.0f) {
            return 1;
        }
    }


    return 0;
}

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
    guPerspectiveF(matrix, perspectiveNormalize, camera->fov, aspectRatio, camera->nearPlane, camera->farPlane, 1.0f);
}

void cameraExtractClippingPlane(float viewPersp[4][4], struct Plane* output, int axis, float direction) {
    output->normal.x = viewPersp[0][axis] * direction + viewPersp[0][3];
    output->normal.y = viewPersp[1][axis] * direction + viewPersp[1][3];
    output->normal.z = viewPersp[2][axis] * direction + viewPersp[2][3];
    output->d = viewPersp[3][axis] * direction + viewPersp[3][3];

    float mult = 1.0f / sqrtf(vector3MagSqrd(&output->normal));
    vector3Scale(&output->normal, &output->normal, mult);
    output->d *= mult;
}

int cameraIsValidMatrix(float matrix[4][4]) {
    return fabsf(matrix[3][0]) <= 0x7fff && fabsf(matrix[3][1]) <= 0x7fff && fabsf(matrix[3][2]) <= 0x7fff;
}

int cameraSetupMatrices(struct Camera* camera, struct RenderState* renderState, float aspectRatio, Vp* viewport, int extractClippingPlanes, struct CameraMatrixInfo* output) {
    float view[4][4];
    float persp[4][4];
    float combined[4][4];

    float scaleX = viewport->vp.vscale[0] * (1.0f / (SCREEN_WD << 1));
    float scaleY = viewport->vp.vscale[1] * (1.0f / (SCREEN_HT << 1));

    float centerX = ((float)viewport->vp.vtrans[0] - (SCREEN_WD << 1)) * (1.0f / (SCREEN_WD << 1));
    float centerY = ((SCREEN_HT << 1) - (float)viewport->vp.vtrans[1]) * (1.0f / (SCREEN_HT << 1));

    guOrthoF(combined, centerX - scaleX, centerX + scaleX, centerY - scaleY, centerY + scaleY, 1.0f, -1.0f, 1.0f);
    cameraBuildProjectionMatrix(camera, view, &output->perspectiveNormalize, aspectRatio);
    guMtxCatF(view, combined, persp);

    cameraBuildViewMatrix(camera, view);
    guMtxCatF(view, persp, combined);

    if (!cameraIsValidMatrix(combined)) {
        goto error;
    }

    output->projectionView = renderStateRequestMatrices(renderState, 1);

    if (!output->projectionView) {
        return 0;
    }

    guMtxF2L(combined, output->projectionView);

    if (extractClippingPlanes) {
        cameraExtractClippingPlane(combined, &output->cullingInformation.clippingPlanes[CLIPPING_PLANE_RIGHT],  0,  1.0f);
        cameraExtractClippingPlane(combined, &output->cullingInformation.clippingPlanes[CLIPPING_PLANE_LEFT],   0, -1.0f);
        cameraExtractClippingPlane(combined, &output->cullingInformation.clippingPlanes[CLIPPING_PLANE_TOP],    1,  1.0f);
        cameraExtractClippingPlane(combined, &output->cullingInformation.clippingPlanes[CLIPPING_PLANE_BOTTOM], 1, -1.0f);
        cameraExtractClippingPlane(combined, &output->cullingInformation.clippingPlanes[CLIPPING_PLANE_NEAR],   2,  1.0f);
        output->cullingInformation.cameraPos = camera->transform.position;
        output->cullingInformation.usedClippingPlaneCount = 5;
    }

    return 1;
error:
    return 0;
}

void cameraModifyProjectionViewForPortalGun(struct Camera* camera, struct RenderState* renderState, float newNearPlane, float aspectRatio)
{
    Mtx* portalGunProjectionView = renderStateRequestMatrices(renderState, 1);
    if(!portalGunProjectionView)
        return;
    
    struct Camera portalCam = *camera;
    portalCam.nearPlane = newNearPlane;
    portalCam.transform.position = gZeroVec;
    float view[4][4];
    float projectionView[4][4];
    unsigned short perspectiveNormalize;
    cameraBuildProjectionMatrix(&portalCam, projectionView, &perspectiveNormalize, aspectRatio);
    cameraBuildViewMatrix(&portalCam, view);
    guMtxCatF(view, projectionView, projectionView);
    
    guMtxF2L(projectionView, portalGunProjectionView);
    gSPMatrix(renderState->dl++, osVirtualToPhysical(portalGunProjectionView), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(renderState->dl++, perspectiveNormalize);
}

int cameraApplyMatrices(struct RenderState* renderState, struct CameraMatrixInfo* matrixInfo) {
    Mtx* modelMatrix = renderStateRequestMatrices(renderState, 1);
    
    if (!modelMatrix) {
        return 0;
    }

    guMtxIdent(modelMatrix);
    gSPMatrix(renderState->dl++, osVirtualToPhysical(modelMatrix), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);

    gSPMatrix(renderState->dl++, osVirtualToPhysical(matrixInfo->projectionView), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(renderState->dl++, matrixInfo->perspectiveNormalize);

    return 1;
}

// assuming projection matrix works as follows
// a 0 0                    0
// 0 b 0                    0
// 0 0 (n + f) / (n - f)    2 * n * f / (n - f)
// 0 0 -1                   0

// distance should be a positive value not scaled by scene scale
// returns -1 for the near plane
// returns 1 for the far plane
float cameraClipDistance(struct Camera* camera, float distance) {
    float modifiedDistance = distance * -SCENE_SCALE;
    
    float denom = modifiedDistance * (camera->nearPlane - camera->farPlane);

    if (fabsf(denom) < 0.00000001f) {
        return 0.0f;
    }

    return -((camera->nearPlane + camera->farPlane) * modifiedDistance + 2.0f * camera->nearPlane * camera->farPlane) / denom;
}

int fogIntValue(float floatValue) {
    if (floatValue < -1.0) {
        return 0;
    }

    if (floatValue > 1.0) {
        return 1000;
    }

    return (int)((floatValue + 1.0f) * 500.0f);
}
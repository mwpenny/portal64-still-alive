
#include "shadow_renderer.h"
#include "util/memory.h"
#include "defs.h"

#define MATRIX_TRANSFORM_SEGMENT    0xC
#define TOP_MATRIX_INDEX        0
#define BOTTOM_MATRIX_INDEX     1

#define SHADOW_COMBINE_MODE     0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE

#define	RM_UPDATE_Z(clk)		\
    Z_CMP | Z_UPD | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_XLU |                          \
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

Gfx shadow_mat[] = {
    gsDPPipeSync(),
    gsSPClearGeometryMode(G_LIGHTING),
    gsDPSetCombineMode(SHADOW_COMBINE_MODE, SHADOW_COMBINE_MODE),
    gsDPSetPrimColor(255, 255, 128, 128, 128, 0),
    gsSPEndDisplayList(),

};

void shadowRendererNewGfxFromOutline(struct Vector2* outline, unsigned pointCount, Gfx** gfxOut, Vtx** vtxOut) {
    Vtx* vtxResult = malloc(sizeof(Vtx) * pointCount);
    Gfx* gfxResult = malloc(sizeof(Gfx) * (7 + pointCount + (pointCount - 1) / 2));

    for (unsigned i = 0; i < pointCount; ++i) {
        vtxResult[i].n.ob[0] = (short)outline[i].x;
        vtxResult[i].n.ob[1] = 0;
        vtxResult[i].n.ob[2] = (short)outline[i].y;
        vtxResult[i].n.flag = 0;

        vtxResult[i].n.tc[0] = 0;
        vtxResult[i].n.tc[1] = 0;

        vtxResult[i].n.n[0] = 0;
        vtxResult[i].n.n[1] = 127;
        vtxResult[i].n.n[2] = 127;
        vtxResult[i].n.a = 255;
    }

    Gfx* gfxCurrent = gfxResult;

    gSPMatrix(gfxCurrent++, (Mtx*)(MATRIX_TRANSFORM_SEGMENT << 24) + TOP_MATRIX_INDEX, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    gSPVertex(gfxCurrent++, vtxResult, pointCount, 0);
    gSPPopMatrix(gfxCurrent++, G_MTX_MODELVIEW);
    
    gSPMatrix(gfxCurrent++, (Mtx*)(MATRIX_TRANSFORM_SEGMENT << 24) + BOTTOM_MATRIX_INDEX, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    gSPVertex(gfxCurrent++, vtxResult, pointCount, pointCount);
    gSPPopMatrix(gfxCurrent++, G_MTX_MODELVIEW);

    for (unsigned i = 0; i < pointCount; ++i) {
        unsigned curr = i;
        unsigned next = (i + 1) % pointCount;
        unsigned bottomCurr = i + pointCount;
        unsigned bottomNext = next + pointCount;
        gSP2Triangles(
            gfxCurrent++, 
            next, 
            curr, 
            bottomNext, 
            0, 
            bottomCurr, 
            bottomNext, 
            curr, 
            0
        );
    }

    for (unsigned i = 1; i + 1 < pointCount; i += 2) {
        if (i + 2 < pointCount) {
            gSP2Triangles(
                gfxCurrent++, 
                pointCount, 
                pointCount + i, 
                pointCount + i + 1,
                0,
                pointCount,
                pointCount + i + 1,
                pointCount + i + 2,
                0
            );
        } else {
            gSP1Triangle(
                gfxCurrent++,
                pointCount,
                pointCount + 1,
                pointCount + i + 1,
                0
            );
        }
    }

    gSPEndDisplayList(gfxCurrent++);

    *gfxOut = gfxResult;
    if (vtxOut) {
        *vtxOut = vtxResult;
    }
}

void shadowRendererGenerateProfile(struct ShadowRenderer* shadowRenderer, unsigned pointCount) {
    shadowRenderer->shadowProfile = malloc(sizeof(Gfx) * (2 + (pointCount - 1) / 2));
    Gfx* gfxCurrent = shadowRenderer->shadowProfile;
    gSPVertex(gfxCurrent++, shadowRenderer->vertices, pointCount, 0);
    for (unsigned i = 1; i + 1 < pointCount; i += 2) {
        if (i + 2 < pointCount) {
            gSP2Triangles(
                gfxCurrent++, 
                0, 
                i, 
                i + 1,
                0,
                0,
                i + 1,
                i + 2,
                0
            );
        } else {
            gSP1Triangle(
                gfxCurrent++,
                pointCount,
                pointCount + 1,
                pointCount + i + 1,
                0
            );
        }
    }

    gSPEndDisplayList(gfxCurrent++);
}

void shadowRendererInit(struct ShadowRenderer* shadowRenderer, struct Vector2* outline, unsigned pointCount, float shadowLength) {
    shadowRenderer->shadowLength = shadowLength;
    transformInitIdentity(&shadowRenderer->casterTransform);
    shadowRendererNewGfxFromOutline(
        outline, 
        pointCount, 
        &shadowRenderer->shadowVolume, 
        &shadowRenderer->vertices
    );
    shadowRendererGenerateProfile(shadowRenderer, pointCount);
}

void shadowRendererRender(
    struct ShadowRenderer* shadowRenderer, 
    struct RenderState* renderState,
    struct PointLight* fromLight, 
    struct ShadowReceiver* recievers, 
    unsigned recieverCount
) {
    Mtx* recieverMatrices = renderStateRequestMatrices(renderState, recieverCount);

    if (!recieverMatrices) {
        return;
    }
 
    unsigned lightCount = 0;

    for (unsigned i = 0; i < recieverCount; ++i) {
        struct ShadowReceiver* reciever = &recievers[i];
        transformToMatrixL(&reciever->transform, &recieverMatrices[i], SCENE_SCALE);

        if (reciever->flags & ShadowReceiverFlagsUseLight) {
            ++lightCount;
        }
    }
    
    Light* lights = renderStateRequestLights(renderState, lightCount);

    if (!lights) {
        return;
    }
    
    unsigned currentLight = 0;

    // first pass for shadowed objects
    for (unsigned i = 0; i < recieverCount; ++i) {
        struct ShadowReceiver* reciever = &recievers[i];

        gSPMatrix(renderState->dl++, &recieverMatrices[i], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        gSPDisplayList(renderState->dl++, reciever->litMaterial);

        if (reciever->flags & ShadowReceiverFlagsUseLight) {
            Light* currLight = &lights[currentLight];
            pointLightCalculateLight(fromLight, &reciever->transform.position, currLight);
            gSPLight(renderState->dl++, currLight, 1);

            ++currentLight;
        }

        gSPDisplayList(renderState->dl++, reciever->geometry);
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
    }

    // calculate position of top and bottom of shadow
    Mtx* shadowMatrices = renderStateRequestMatrices(renderState, 2);
    if (!shadowMatrices) {
        return;
    }
    transformToMatrixL(&shadowRenderer->casterTransform, &shadowMatrices[TOP_MATRIX_INDEX], SCENE_SCALE);

    struct Vector3 lightOffset;
    vector3Sub(&shadowRenderer->casterTransform.position, &fromLight->position, &lightOffset);

    float lightDistance = sqrtf(vector3MagSqrd(&lightOffset));

    struct Vector3 shadowUp;
    quatMultVector(&shadowRenderer->casterTransform.rotation, &gUp, &shadowUp);
    float lightVerticalOffset = vector3Dot(&shadowUp, &lightOffset);

    struct Transform shadowEnd;
    shadowEnd.rotation = shadowRenderer->casterTransform.rotation;
    vector3AddScaled(
        &shadowRenderer->casterTransform.position, 
        &lightOffset, 
        shadowRenderer->shadowLength / lightDistance,
        &shadowEnd.position
    );
    vector3Scale(&gOneVec, &shadowEnd.scale, (lightDistance + shadowRenderer->shadowLength) / lightDistance);
    transformToMatrixL(&shadowEnd, &shadowMatrices[BOTTOM_MATRIX_INDEX], SCENE_SCALE);

    // render back of shadows
    gDPPipeSync(renderState->dl++);
    // check if the shadow is inside out and need culling to flipped
    if (lightVerticalOffset < 0.0f) {
        gSPGeometryMode(renderState->dl++, G_CULL_BACK, G_CULL_FRONT);
    }
    gDPSetRenderMode(renderState->dl++, RM_UPDATE_Z(1), RM_UPDATE_Z(2));
    gSPSegment(renderState->dl++, MATRIX_TRANSFORM_SEGMENT, shadowMatrices);
    gSPDisplayList(renderState->dl++, shadow_mat);
    gSPDisplayList(renderState->dl++, shadowRenderer->shadowVolume);


    gDPPipeSync(renderState->dl++);
    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_DECAL, G_RM_ZB_OPA_DECAL2);

    if (lightVerticalOffset < 0.0f) {
        gSPGeometryMode(renderState->dl++, G_CULL_FRONT, G_CULL_BACK);
    }
    // second pass for shadowed objects
    for (unsigned i = 0; i < recieverCount; ++i) {
        struct ShadowReceiver* reciever = &recievers[i];

        gSPMatrix(renderState->dl++, &recieverMatrices[i], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        gSPDisplayList(renderState->dl++, reciever->shadowMaterial);

        if (reciever->flags & ShadowReceiverFlagsUseLight) {
            gSPLight(renderState->dl++, &gLightBlack, 1);
        }

        gSPDisplayList(renderState->dl++, reciever->geometry);
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
    }

    // render front of shadows
    gDPPipeSync(renderState->dl++);
    // check if the shadow is inside out and need culling to flipped
    if (lightVerticalOffset > 0.0f) {
        gSPGeometryMode(renderState->dl++, G_CULL_BACK, G_CULL_FRONT);
    }
    gDPSetRenderMode(renderState->dl++, RM_UPDATE_Z(1), RM_UPDATE_Z(2));
    gSPSegment(renderState->dl++, MATRIX_TRANSFORM_SEGMENT, shadowMatrices);
    gSPDisplayList(renderState->dl++, shadow_mat);
    gSPDisplayList(renderState->dl++, shadowRenderer->shadowVolume);

    gDPPipeSync(renderState->dl++);
    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_DECAL, G_RM_ZB_OPA_DECAL2);
    if (lightVerticalOffset > 0.0f) {
        gSPGeometryMode(renderState->dl++, G_CULL_FRONT, G_CULL_BACK);
    }

    // third pass for shadowed objects
    currentLight = 0;
    for (unsigned i = 0; i < recieverCount; ++i) {
        struct ShadowReceiver* reciever = &recievers[i];

        gSPMatrix(renderState->dl++, &recieverMatrices[i], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        gSPDisplayList(renderState->dl++, reciever->litMaterial);

        if (reciever->flags & ShadowReceiverFlagsUseLight) {
            gSPLight(renderState->dl++, &lights[currentLight], 1);
            ++currentLight;
        }

        gSPDisplayList(renderState->dl++, reciever->geometry);
        gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
    }
}

void shadowRendererRenderProjection(
    struct ShadowRenderer* shadowRenderer, 
    struct RenderState* renderState, 
    struct PointLight* fromLight,
    struct Vector3* toPoint,
    struct Vector3* normal
) {
    struct Vector3 offset;
    vector3Sub(toPoint, &fromLight->position, &offset);
    float distToFloor = vector3Dot(&offset, normal);

    vector3Sub(&shadowRenderer->casterTransform.position, &fromLight->position, &offset);
    float distToCaster = vector3Dot(&offset, normal);

    float uniformScale = distToFloor / distToCaster;

    struct Transform finalTransform;
    vector3AddScaled(&fromLight->position, &offset, uniformScale, &finalTransform.position);
    finalTransform.rotation = shadowRenderer->casterTransform.rotation;
    vector3Scale(&gOneVec, &finalTransform.scale, uniformScale);

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&finalTransform, matrix, SCENE_SCALE);
    
    gSPMatrix(renderState->dl++, matrix, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
    gSPDisplayList(renderState->dl++, shadowRenderer->shadowProfile);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}
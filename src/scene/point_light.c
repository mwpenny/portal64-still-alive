
#include "point_light.h"
#include "defs.h"
#include "util/memory.h"
#include "math/mathf.h"
#include "graphics/graphics.h"
#include "defs.h"

Light gLightBlack = {{{0, 0, 0}, 0, {0, 0, 0}, 0, {0, 0x7f, 0}, 0}};

void pointLightRecalcMaxFactor(struct PointLight* pointLight) {
    float result = 10000000.0f;
    
    if (pointLight->color.r) {
        result = MIN(result, 255.0f / pointLight->color.r);
    }

    if (pointLight->color.g) {
        result = MIN(result, 255.0f / pointLight->color.g);
    }

    if (pointLight->color.b) {
        result = MIN(result, 255.0f / pointLight->color.b);
    }

    pointLight->maxFactor = result;
}

void pointLightInit(struct PointLight* pointLight, struct Vector3* position, struct Coloru8* color, float intensity) {
    pointLight->position = *position;
    pointLight->color = *color;
    pointLight->intensity = intensity;
    pointLightRecalcMaxFactor(pointLight);
}

void pointLightSetColor(struct PointLight* pointLight, struct Coloru8* color) {
    pointLight->color = *color;
    pointLightRecalcMaxFactor(pointLight);
}

void pointLightAttenuate(struct PointLight* pointLight, float distnaceSq, struct Coloru8* output) {
    float factor = pointLight->intensity / distnaceSq;

    factor = MIN(factor, pointLight->maxFactor);
    colorU8Lerp(&gColorBlack, &pointLight->color, factor, output);
}

void pointLightCalculateLight(struct PointLight* pointLight, struct Vector3* target, Light* output) {
    struct Vector3 normalized;
    struct Vector3 offset;
    vector3Sub(&pointLight->position, target, &offset);
    vector3Normalize(&offset, &normalized);
    vector3ToVector3u8(&normalized, (struct Vector3u8*)output->l.dir);

    struct Coloru8 finalColor;
    pointLightAttenuate(pointLight, vector3MagSqrd(&offset), &finalColor);

    output->l.col[0] = finalColor.r;
    output->l.col[1] = finalColor.g;
    output->l.col[2] = finalColor.b;

    output->l.colc[0] = finalColor.r;
    output->l.colc[1] = finalColor.g;
    output->l.colc[2] = finalColor.b;
}

#define MAX_REASONABLE_VERTICES     1000

#define LIGHTING_COMBINE_MODE   TEXEL0, 0, SHADE, ENVIRONMENT, 0, 0, 0, ENVIRONMENT
#define LIGHTING_COMBINE_MODE_IN_SHADOW   0, 0, 0, ENVIRONMENT, 0, 0, 0, ENVIRONMENT

void pointLightableSetMaterial(struct PointLightableMesh* mesh, struct RenderState* renderState, struct Coloru8* ambient) {
    struct Coloru8 finalColor;
    colorU8Mul(ambient, &mesh->color, &finalColor);
    gDPSetEnvColor(renderState->dl++, finalColor.r, finalColor.g, finalColor.b, finalColor.a);
    gDPSetCombineMode(renderState->dl++, LIGHTING_COMBINE_MODE, LIGHTING_COMBINE_MODE);
    gSPGeometryMode(renderState->dl++, G_LIGHTING, G_SHADE);
}

void pointLightableSetMaterialInShadow(struct PointLightableMesh* mesh, struct RenderState* renderState, struct Coloru8* ambient) {
    struct Coloru8 finalColor;
    colorU8Mul(ambient, &mesh->color, &finalColor);
    gDPSetEnvColor(renderState->dl++, finalColor.r, finalColor.g, finalColor.b, finalColor.a);
    gDPSetCombineMode(renderState->dl++, LIGHTING_COMBINE_MODE_IN_SHADOW, LIGHTING_COMBINE_MODE_IN_SHADOW);
    gSPGeometryMode(renderState->dl++, G_LIGHTING, G_SHADE);
}

void pointLightableMeshInit(struct PointLightableMesh* mesh, Vtx* inputVertices, Gfx* drawCommand, struct Coloru8* color) {
    unsigned vertexCount = 0;
    // start at 1 for the gSPEndDisplayList at the end
    unsigned gfxCommandCount = 1;

    mesh->color = *color;

    for (Gfx* curr = drawCommand; GET_GFX_TYPE(curr) != G_ENDDL; ++curr) {
        unsigned type = GET_GFX_TYPE(curr);

        switch (type) {
            case G_VTX:
            {
                unsigned base = (Vtx*)curr->words.w1 - inputVertices;
                base += _SHIFTR(curr->words.w0,12,8);
                vertexCount = MAX(vertexCount, base);
                break;
            }
        }

        ++gfxCommandCount;
    }

    mesh->vertexNormals = malloc(sizeof(struct Vector3) * vertexCount);
    mesh->vertexTangents = malloc(sizeof(struct Vector3) * vertexCount);
    mesh->vertexCoTangents = malloc(sizeof(struct Vector3) * vertexCount);
    mesh->oututVertices = malloc(sizeof(Vtx) * vertexCount);
    mesh->inputVertices = inputVertices;
    mesh->drawCommand = malloc(sizeof(Gfx) * gfxCommandCount);
    mesh->vertexCount = vertexCount;

    for (unsigned i = 0; i < vertexCount; ++i) {
        struct Vector3* normal = &mesh->vertexNormals[i];
        normal->x = inputVertices[i].n.n[0];
        normal->y = inputVertices[i].n.n[1];
        normal->z = inputVertices[i].n.n[2];
        vector3Normalize(normal, normal);

        float rightDot = vector3Dot(normal, &gRight);
        float forwardDot = vector3Dot(normal, &gUp);

        struct Vector3* tangent = &mesh->vertexTangents[i];

        if (fabsf(rightDot) < fabsf(forwardDot)) {
            vector3ProjectPlane(&gRight, normal, tangent);
        } else {
            vector3ProjectPlane(&gUp, normal, tangent);
        }
        vector3Normalize(tangent, tangent);

        mesh->oututVertices[i] = mesh->inputVertices[i];

        vector3Cross(normal, tangent, &mesh->vertexCoTangents[i]);
    }

    Gfx* currGfx = mesh->drawCommand;

    for (unsigned i = 0; i < gfxCommandCount; ++i) {
        *currGfx = drawCommand[i];
        if (GET_GFX_TYPE(currGfx) == G_VTX) {
            currGfx->words.w1 = (unsigned)(((Vtx*)currGfx->words.w1 - inputVertices) + mesh->oututVertices);
        }

        ++currGfx;
    }
}

#define RENDERED_LIGHT_HEIGHT       (0.5f)
#define RENDERED_LIGHT_TEX_SIZE     (4.0f)
#define RENDERED_LIGHT_TEX_UV_SIZE  4096

void pointLightableCalc(struct PointLightableMesh* mesh, struct Transform* meshTransform, struct PointLight* pointLight) {
    struct Vector3 relativePos;
    transformPointInverseNoScale(meshTransform, &pointLight->position, &relativePos);

    for (unsigned i = 0; i < mesh->vertexCount; ++i) {
        struct Vector3 vertexPos;
        Vtx_tn* input = &mesh->inputVertices[i].n;
        vertexPos.x = input->ob[0];
        vertexPos.y = input->ob[1];
        vertexPos.z = input->ob[2];

        struct Vector3 offset;
        vector3Sub(&relativePos, &vertexPos, &offset);

        float perpDistnace = vector3Dot(&mesh->vertexNormals[i], &offset);
        float tangentDistnace = vector3Dot(&mesh->vertexTangents[i], &offset);
        float coTangentDistnace = vector3Dot(&mesh->vertexCoTangents[i], &offset);

        float uvWorldSize = RENDERED_LIGHT_TEX_SIZE * perpDistnace / RENDERED_LIGHT_HEIGHT;

        Vtx_t* output = &mesh->oututVertices[i].v;
        struct Coloru8 col;
        if (uvWorldSize < 0.0001f) {
            output->tc[0] = 0;
            output->tc[1] = 0;
            col = gColorBlack;
        } else {
            float u = -(tangentDistnace / uvWorldSize - 0.5f) * RENDERED_LIGHT_TEX_UV_SIZE;
            float v = -(coTangentDistnace / uvWorldSize - 0.5f) * RENDERED_LIGHT_TEX_UV_SIZE;

            output->tc[0] = (short)u;
            output->tc[1] = (short)v;
            pointLightAttenuate(pointLight, perpDistnace * perpDistnace, &col);
        }

        colorU8Mul(&col, &mesh->color, &col);

        output->cn[0] = col.r;
        output->cn[1] = col.g;
        output->cn[2] = col.b;
        output->cn[3] = col.a;
    }

    osWritebackDCache(mesh->oututVertices, sizeof(Vtx) * mesh->vertexCount);
}
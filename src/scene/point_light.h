#ifndef __POINT_LIGHT_H__
#define __POINT_LIGHT_H__

#include <ultra64.h>

#include "math/vector3.h"
#include "math/transform.h"
#include "graphics/color.h"
#include "graphics/renderstate.h"

struct PointLight {
    struct Vector3 position;
    struct Coloru8 color;
    float intensity;
    float maxFactor;
};

struct PointLightableMesh {
    struct Vector3* vertexNormals;
    struct Vector3* vertexTangents;
    struct Vector3* vertexBitangents;
    Vtx* inputVertices;
    Vtx* oututVertices;
    Gfx* drawCommand;
    unsigned vertexCount;
    struct Coloru8 color;
};

extern Light gLightBlack;

void pointLightInit(struct PointLight* pointLight, struct Vector3* position, struct Coloru8* color, float intensity);
void pointLightCalculateLight(struct PointLight* pointLight, struct Vector3* target, Light* output);
void pointLightSetColor(struct PointLight* pointLight, struct Coloru8* color);

void pointLightableSetMaterial(struct PointLightableMesh* mesh, struct RenderState* renderState, struct Coloru8* ambient);
void pointLightableSetMaterialInShadow(struct PointLightableMesh* mesh, struct RenderState* renderState, struct Coloru8* ambient);
void pointLightableMeshInit(struct PointLightableMesh* mesh, Vtx* inputVertices, Gfx* drawCommand, struct Coloru8* color);
void pointLightableCalc(struct PointLightableMesh* mesh, struct Transform* meshTransform, struct PointLight* pointLight);

#endif
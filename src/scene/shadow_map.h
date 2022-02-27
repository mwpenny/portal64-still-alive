#ifndef __SHADOW_MAP_H__
#define __SHADOW_MAP_H__

#include <ultra64.h>
#include "graphics/graphics.h"
#include "math/transform.h"
#include "math/plane.h"
#include "shadow_map.h"
#include "point_light.h"

struct ShadowMap {
    Gfx* subject;
    float subjectRadius;
    struct Coloru8 shadowColor;
};

void shadowMapInit(struct ShadowMap* shadowMap, Gfx* subject, struct Coloru8 shadowColor);
void shadowMapRender(struct ShadowMap* shadowMap, struct RenderState* renderState, struct GraphicsTask* gfxTask, struct PointLight* from, struct Transform* subjectTransform, struct Plane* onto);
void shadowMapRenderDebug(struct RenderState* renderState);


#endif
#ifndef __DEBUG_RENDER_H__
#define __DEBUG_RENDER_H__

#include "../math/vector3.h"
#include "renderstate.h"

void debugRenderQuad(struct Vector3* origin, struct Vector3* edgeA, struct Vector3* edgeB, float edgeLengthA, float edgeLengthB, struct RenderState* renderState);

#endif
#ifndef __MATERIAL_STATE_H__
#define __MATERIAL_STATE_H__

#include "../graphics/renderstate.h"

struct MaterialState {
    short materialIndex;
};

void materialStateInit(struct MaterialState* state, short initialMaterial);
void materialStateSet(struct MaterialState* state, short materialIndex, struct RenderState* renderState);

#endif
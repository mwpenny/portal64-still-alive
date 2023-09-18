#include "material_state.h"

#include "levels.h"

void materialStateInit(struct MaterialState* state, short initialMaterial) {
    state->materialIndex = initialMaterial;
}

void materialStateSet(struct MaterialState* state, short materialIndex, struct RenderState* renderState) {
    if (state->materialIndex != materialIndex) {
        if (state->materialIndex != -1) {
            gSPDisplayList(renderState->dl++, levelMaterialRevert(state->materialIndex));
        }

        state->materialIndex = materialIndex;

        gSPDisplayList(renderState->dl++, levelMaterial(materialIndex));
    }
}
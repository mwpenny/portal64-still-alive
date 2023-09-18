#include "portal_trail.h"

#include "../math/vector2.h"
#include "../defs.h"

#include "../build/assets/models/portal_gun/ball_trail.h"
#include "../build/assets/materials/static.h"
#include "../graphics/color.h"

struct Transform gTrailSectionOffset = {
    {0.0f, 0.0f, -8.0f},
    {0.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
};

void portalTrailInit(struct PortalTrail* trail) {
    transformToMatrixL(&gTrailSectionOffset, &trail->sectionOffset, SCENE_SCALE);
    guMtxIdent(&trail->baseTransform[0]);
    guMtxIdent(&trail->baseTransform[1]);

    trail->currentBaseTransform = 0;
}

void portalTrailPlay(struct PortalTrail* trail, struct Vector3* from, struct Vector3* to) {
    trail->trailTransform.position = *from;
    struct Vector3 dir;
    vector3Sub(to, from, &dir);
    quatLook(&dir, &gUp, &trail->trailTransform.rotation);
    trail->trailTransform.scale = gOneVec;

    trail->currentBaseTransform ^= 1;

    transformToMatrixL(&trail->trailTransform, &trail->baseTransform[trail->currentBaseTransform], SCENE_SCALE);
    osWritebackDCache(&trail->baseTransform[trail->currentBaseTransform], sizeof(Mtx));
}

void portalTrailUpdate(struct PortalTrail* trail, float distance) {

}

struct Coloru8 gTrailColor[] = {
    {200, 100, 50, 255},
    {50, 70, 200, 255},
};

void portalTrailRender(struct PortalTrail* trail, struct RenderState* renderState, struct MaterialState* materialState) {
    materialStateSet(materialState, PORTAL_TRAIL_INDEX, renderState);

    struct Coloru8* color = &gTrailColor[0];
    gSPFogPosition(renderState->dl++, 800, 999);
        gDPSetPrimColor(renderState->dl++, 255, 255, color->r, color->g, color->b, color->a);

    gSPMatrix(renderState->dl++, &trail->baseTransform[trail->currentBaseTransform], G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);

    gSPDisplayList(renderState->dl++, portal_gun_ball_trail_model_gfx);

    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}
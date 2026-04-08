#include "doorway_cover.h"

#include "math/matrix.h"
#include "physics/collision_scene.h"
#include "scene/dynamic_scene.h"

#include "codegen/assets/materials/static.h"

static Vtx cover_vertices[] = {
    {{{-SCENE_SCALE / 2, -SCENE_SCALE / 2, 0}, 0, {0, 0}, {0, 0, 0, 0}}},
    {{{ SCENE_SCALE / 2, -SCENE_SCALE / 2, 0}, 0, {0, 0}, {0, 0, 0, 0}}},
    {{{ SCENE_SCALE / 2,  SCENE_SCALE / 2, 0}, 0, {0, 0}, {0, 0, 0, 0}}},
    {{{-SCENE_SCALE / 2,  SCENE_SCALE / 2, 0}, 0, {0, 0}, {0, 0, 0, 0}}},
};

static Gfx cover_gfx[] = {
    gsSPVertex(cover_vertices, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList()
};

static float calculateOpacity(struct DoorwayCover* cover, struct Vector3* offset) {
    float dist = vector3MagSqrd(offset);
    float start = cover->definition->fadeStartDistance;
    float end = cover->definition->fadeEndDistance;

    if (dist <= (start * start)) {
        return 0.0f;
    } else if (dist >= (end * end)) {
        return 1.0f;
    } else {
        return mathfInvLerp(start, end, sqrtf(dist));
    }
}

static float calculateAxisOpacity(struct DoorwayCover* cover, struct Vector3* offset) {
    float dist = fabsf(vector3Dot(offset, &cover->definition->fadeAxis));
    float start = cover->definition->fadeStartDistance;
    float end = cover->definition->fadeEndDistance;

    if (dist <= start) {
        return 0.0f;
    } else if (dist >= end) {
        return 1.0f;
    } else {
        return mathfInvLerp(start, end, dist);
    }
}

static float doorwayCoverOpacity(struct DoorwayCover* cover, u64* visibleRooms, struct Vector3* viewPosition) {
    if ((*visibleRooms & cover->roomFlags) != cover->roomFlags) {
        // One of the rooms isn't visible, but getting here means the cover is.
        // Either the room is explicitly hidden or the fade distance is passed.
        // In both cases the cover should render opaque to hide the void.
        return 1.0f;
    }

    if (cover->definition->fadeEndDistance < 0.0f) {
        return 0.0f;
    }

    struct Vector3 offset;
    vector3Sub(&cover->definition->position, viewPosition, &offset);

    if (!vector3IsZero(&cover->definition->fadeAxis)) {
        return calculateAxisOpacity(cover, &offset);
    } else {
        return calculateOpacity(cover, &offset);
    }
}

static void doorwayCoverRender(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct DoorwayCover* cover = (struct DoorwayCover*)data;

    // It's difficult to use fog here since portals are their own cameras. Near
    // and far plane distances are relative to each camera, leading to incorrect
    // fade amounts depending on portal placement. CPU-side distance calculation
    // is required regardless to compensate, so simply calculate fade instead.
    float opacity = doorwayCoverOpacity(cover, &renderScene->visibleRooms, &fromView->position);
    if (opacity <= 0.0f) {
        return;
    }

    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    if (!matrix) {
        return;
    }

    Gfx* dl = renderStateAllocateDLChunk(renderScene->renderState, 3);
    Gfx* curr = dl;
    if (!dl) {
        return;
    }

    matrixFromBasisL(
        matrix,
        &cover->definition->position,
        &cover->definition->basis.x,
        &cover->definition->basis.y,
        &cover->definition->basis.z
    );

    struct Coloru8* color = &cover->definition->color;

    gDPSetEnvColor(curr++, color->r, color->g, color->b, opacity * 255.0f);
    gSPDisplayList(curr++, cover_gfx);
    gSPEndDisplayList(curr++);

    renderSceneAdd(
        renderScene,
        dl,
        matrix,
        DOORWAY_COVER_INDEX,
        &cover->definition->position,
        NULL
    );
}

void doorwayCoverInit(struct DoorwayCover* cover, struct DoorwayCoverDefinition* definition, struct World* world) {
    cover->forDoorway = &world->doorways[definition->doorwayIndex];
    cover->roomFlags = ROOM_FLAG_FROM_INDEX(cover->forDoorway->roomA) | ROOM_FLAG_FROM_INDEX(cover->forDoorway->roomB);
    cover->definition = definition;

    float radius = sqrtf(
        (cover->forDoorway->quad.edgeALength * cover->forDoorway->quad.edgeALength) +
        (cover->forDoorway->quad.edgeBLength * cover->forDoorway->quad.edgeBLength)
    ) * 0.5f;
    cover->dynamicId = dynamicSceneAddViewDependent(cover, doorwayCoverRender, &definition->position, radius);
    dynamicSceneSetRoomFlags(cover->dynamicId, cover->roomFlags);
}

int doorwayCoverIsOpaqueFromView(struct DoorwayCover* cover, struct Vector3* viewPosition) {
    if (cover->definition->fadeEndDistance < 0.0f) {
        return 0;
    }

    struct Vector3 offset;
    vector3Sub(&cover->definition->position, viewPosition, &offset);

    if (!vector3IsZero(&cover->definition->fadeAxis)) {
        float threshold = cover->definition->fadeEndDistance;
        return fabsf(vector3Dot(&offset, &cover->definition->fadeAxis)) >= threshold;
    } else {
        float threshold = cover->definition->fadeEndDistance * cover->definition->fadeEndDistance;
        return vector3MagSqrd(&offset) >= threshold;
    }
}

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

static void doorwayCoverRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct DoorwayCover* cover = (struct DoorwayCover*)data;

    if (cover->opacity <= 0.0f) {
        return;
    }

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);
    if (!matrix) {
        return;
    }

    Gfx* dl = renderStateAllocateDLChunk(renderState, 3);
    Gfx* curr = dl;
    if (!dl) {
        return;
    }

    struct CollisionQuad* quad = &cover->forDoorway->quad;
    struct Coloru8* color = &cover->definition->color;

    // TODO: precompute
    struct Vector3 normal;
    struct Vector3 x;
    struct Vector3 y;
    vector3Scale(&quad->edgeA, &x, (quad->edgeALength - 1.0f));
    vector3Scale(&quad->edgeB, &y, (quad->edgeBLength - 1.0f));
    vector3Cross(&quad->edgeA, &quad->edgeB, &normal);
    matrixFromBasisL(matrix, &cover->definition->position, &x, &y, &normal);

    gDPSetEnvColor(curr++, color->r, color->g, color->b, cover->opacity * 255);
    gSPDisplayList(curr++, cover_gfx);
    gSPEndDisplayList(curr++);

    dynamicRenderListAddData(
        renderList,
        dl,
        matrix,
        DOORWAY_COVER_INDEX,
        &cover->definition->position,
        NULL
    );
}

void doorwayCoverInit(struct DoorwayCover* cover, struct DoorwayCoverDefinition* definition, struct World* world) {
    cover->forDoorway = &world->doorways[definition->doorwayIndex];
    cover->forDoorway->flags &= ~DoorwayFlagsOpen;
    cover->opacity = 0.0f;

    float radius = sqrtf(
        (cover->forDoorway->quad.edgeALength * cover->forDoorway->quad.edgeALength * 0.25f) +
        (cover->forDoorway->quad.edgeBLength * cover->forDoorway->quad.edgeBLength * 0.25f)
    );
    cover->dynamicId = dynamicSceneAdd(cover, doorwayCoverRender, &definition->position, radius);
    dynamicSceneSetRoomFlags(cover->dynamicId, ROOM_FLAG_FROM_INDEX(cover->forDoorway->roomA) | ROOM_FLAG_FROM_INDEX(cover->forDoorway->roomB));

    cover->definition = definition;
}

void doorwayCoverUpdate(struct DoorwayCover* cover, struct Player* player) {
    // TODO: through portals

    float dist = vector3DistSqrd(&cover->definition->position, &player->body.transform.position);
    float start = cover->definition->fadeStartDistance;
    float end = cover->definition->fadeEndDistance;

    if (dist >= (end * end)) {
        cover->opacity = 1.0f;
        cover->forDoorway->flags &= ~DoorwayFlagsOpen;
    } else if (dist <= (start * start)) {
        cover->opacity = 0.0f;
        cover->forDoorway->flags |= DoorwayFlagsOpen;
    } else {
        dist = clampf(sqrtf(dist), start, end) - start;
        cover->opacity = dist / (end - start);
        cover->forDoorway->flags |= DoorwayFlagsOpen;
    }
}

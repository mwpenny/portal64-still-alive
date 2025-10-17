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

    matrixFromBasisL(
        matrix,
        &cover->definition->position,
        &cover->definition->basis.x,
        &cover->definition->basis.y,
        &cover->definition->basis.z
    );

    struct Coloru8* color = &cover->definition->color;

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
    cover->opacity = 0.0f;

    float radius = sqrtf(
        (cover->forDoorway->quad.edgeALength * cover->forDoorway->quad.edgeALength * 0.25f) +
        (cover->forDoorway->quad.edgeBLength * cover->forDoorway->quad.edgeBLength * 0.25f)
    );
    cover->dynamicId = dynamicSceneAdd(cover, doorwayCoverRender, &definition->position, radius);
    dynamicSceneSetRoomFlags(cover->dynamicId, ROOM_FLAG_FROM_INDEX(cover->forDoorway->roomA) | ROOM_FLAG_FROM_INDEX(cover->forDoorway->roomB));

    cover->definition = definition;
}

static float doorwayCoverGetClosestDistance(struct DoorwayCover* cover, struct Player* player, struct Portal* portals) {
    float dist = vector3DistSqrd(&cover->definition->position, &player->body.transform.position);

    if (!collisionSceneIsPortalOpen()) {
        return dist;
    }

    float endSquared = cover->definition->fadeEndDistance * cover->definition->fadeEndDistance;

    if (vector3DistSqrd(&cover->definition->position, &portals[0].rigidBody.transform.position) < endSquared ||
        vector3DistSqrd(&cover->definition->position, &portals[1].rigidBody.transform.position) < endSquared
    ) {
        for (int i = 0; i < 2; ++i) {
            struct Transform relativeTransform;
            collisionSceneGetPortalTransform(i, &relativeTransform);

            struct Vector3 viewPosition;
            transformPointNoScale(&relativeTransform, &player->lookTransform.position, &viewPosition);

            float distThroughPortal = vector3DistSqrd(&cover->definition->position, &viewPosition);
            if (distThroughPortal < dist) {
                dist = distThroughPortal;
            }
        }
    }

    return dist;
}

void doorwayCoverUpdate(struct DoorwayCover* cover, struct Player* player, struct Portal* portals) {
    float dist = doorwayCoverGetClosestDistance(cover, player, portals);
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

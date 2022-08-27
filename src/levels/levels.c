#include "levels.h"

#include "../build/assets/test_chambers/level_list.h"
#include "../build/assets/materials/static.h"

#include "physics/collision_scene.h"
#include "static_render.h"
#include "cutscene_runner.h"
#include "../graphics/graphics.h"
#include "../player/player.h"

#include "../util/rom.h"
#include "../util/memory.h"

struct LevelDefinition* gCurrentLevel;
int gCurrentLevelIndex;
u64 gTriggeredCutscenes;

int gQueuedLevel = NO_QUEUED_LEVEL;
struct Transform gRelativeTransform = {
    {0.0f, PLAYER_HEAD_HEIGHT, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
};

struct Vector3 gRelativeVelocity;

int levelCount() {
    return LEVEL_COUNT;
}

#define ADJUST_POINTER_POS(ptr, offset) (void*)((ptr) ? (char*)(ptr) + (offset) : 0)

struct LevelDefinition* levelFixPointers(struct LevelDefinition* from, int pointerOffset) {
    struct LevelDefinition* result = ADJUST_POINTER_POS(from, pointerOffset);

    result->collisionQuads = ADJUST_POINTER_POS(result->collisionQuads, pointerOffset);

    for (int i = 0; i < result->collisionQuadCount; ++i) {
        result->collisionQuads[i].collider = ADJUST_POINTER_POS(result->collisionQuads[i].collider, pointerOffset);
        result->collisionQuads[i].collider->data = ADJUST_POINTER_POS(result->collisionQuads[i].collider->data, pointerOffset);
        result->collisionQuads[i].body = ADJUST_POINTER_POS(result->collisionQuads[i].body, pointerOffset);
        result->collisionQuads[i].data = ADJUST_POINTER_POS(result->collisionQuads[i].data, pointerOffset);
    }

    result->staticContent = ADJUST_POINTER_POS(result->staticContent, pointerOffset);
    result->roomStaticMapping = ADJUST_POINTER_POS(result->roomStaticMapping, pointerOffset);
    result->staticBoundingBoxes = ADJUST_POINTER_POS(result->staticBoundingBoxes, pointerOffset);
    result->portalSurfaces = ADJUST_POINTER_POS(result->portalSurfaces, pointerOffset);

    for (int i = 0; i < result->portalSurfaceCount; ++i) {
        result->portalSurfaces[i].vertices = ADJUST_POINTER_POS(result->portalSurfaces[i].vertices, pointerOffset);
        result->portalSurfaces[i].edges = ADJUST_POINTER_POS(result->portalSurfaces[i].edges, pointerOffset);
        result->portalSurfaces[i].gfxVertices = ADJUST_POINTER_POS(result->portalSurfaces[i].gfxVertices, pointerOffset);
    }

    result->portalSurfaceMappingRange = ADJUST_POINTER_POS(result->portalSurfaceMappingRange, pointerOffset);
    result->portalSurfaceMappingIndices = ADJUST_POINTER_POS(result->portalSurfaceMappingIndices, pointerOffset);
    result->triggers = ADJUST_POINTER_POS(result->triggers, pointerOffset);
    result->cutscenes = ADJUST_POINTER_POS(result->cutscenes, pointerOffset);

    for (int i = 0; i < result->cutsceneCount; ++i) {
        result->cutscenes[i].steps = ADJUST_POINTER_POS(result->cutscenes[i].steps, pointerOffset);
    }

    result->locations = ADJUST_POINTER_POS(result->locations, pointerOffset);
    result->world.rooms = ADJUST_POINTER_POS(result->world.rooms, pointerOffset);
    result->world.doorways = ADJUST_POINTER_POS(result->world.doorways, pointerOffset);

    for (int i = 0; i < result->world.roomCount; ++i) {
        result->world.rooms[i].quadIndices = ADJUST_POINTER_POS(result->world.rooms[i].quadIndices, pointerOffset);
        result->world.rooms[i].cellContents = ADJUST_POINTER_POS(result->world.rooms[i].cellContents, pointerOffset);
        result->world.rooms[i].doorwayIndices = ADJUST_POINTER_POS(result->world.rooms[i].doorwayIndices, pointerOffset);
    }

    result->doors = ADJUST_POINTER_POS(result->doors, pointerOffset);
    result->buttons = ADJUST_POINTER_POS(result->buttons, pointerOffset);
    result->signalOperators = ADJUST_POINTER_POS(result->signalOperators, pointerOffset);
    result->decor = ADJUST_POINTER_POS(result->decor, pointerOffset);
    result->fizzlers = ADJUST_POINTER_POS(result->fizzlers, pointerOffset);
    result->elevators = ADJUST_POINTER_POS(result->elevators, pointerOffset);
    result->pedestals = ADJUST_POINTER_POS(result->pedestals, pointerOffset);
    result->signage = ADJUST_POINTER_POS(result->signage, pointerOffset);
    result->boxDroppers = ADJUST_POINTER_POS(result->boxDroppers, pointerOffset);

    return result;
}

void levelLoad(int index) {
    if (index < 0 || index >= LEVEL_COUNT) {
        return;
    }

    struct LevelMetadata* metadata = &gLevelList[index];

    void* memory = malloc(metadata->segmentRomEnd - metadata->segmentRomStart);
    romCopy(metadata->segmentRomStart, memory, metadata->segmentRomEnd - metadata->segmentRomStart);

    gLevelSegment = memory;

    gCurrentLevel = levelFixPointers(metadata->levelDefinition, (char*)memory - metadata->segmentStart);
    gCurrentLevelIndex = index;
    gTriggeredCutscenes = 0;
    gQueuedLevel = NO_QUEUED_LEVEL;

    collisionSceneInit(&gCollisionScene, gCurrentLevel->collisionQuads, gCurrentLevel->collisionQuadCount, &gCurrentLevel->world);
}

void levelQueueLoad(int index, struct Transform* relativeExitTransform, struct Vector3* relativeVelocity) {
    if (index == NEXT_LEVEL) {
        gQueuedLevel = gCurrentLevelIndex + 1;
    } else {
        gQueuedLevel = index;
    }
    gRelativeTransform = *relativeExitTransform;
    gRelativeVelocity = *relativeVelocity;
}

int levelGetQueued() {
    return gQueuedLevel;
}

struct Transform* levelRelativeTransform() {
    return &gRelativeTransform;
}

struct Vector3* levelRelativeVelocity() {
    return &gRelativeVelocity;
}

int levelMaterialCount() {
    return STATIC_MATERIAL_COUNT;
}

int levelMaterialTransparentStart() {
    return STATIC_TRANSPARENT_START;
}

Gfx* levelMaterial(int index) {
    if (index < 0 || index >= STATIC_MATERIAL_COUNT) {
        return NULL;
    }

    return static_material_list[index];
}

Gfx* levelMaterialDefault() {
    return static_material_list[DEFAULT_INDEX];
}

Gfx* levelMaterialRevert(int index) {
    if (index < 0 || index >= STATIC_MATERIAL_COUNT) {
        return NULL;
    }

    return static_material_revert_list[index];
}

int levelQuadIndex(struct CollisionObject* pointer) {
    if (pointer < gCollisionScene.quads || pointer >= gCollisionScene.quads + gCollisionScene.quadCount) {
        return -1;
    }

    return pointer - gCollisionScene.quads;
}

void levelCheckTriggers(struct Vector3* playerPos) {
    for (int i = 0; i < gCurrentLevel->triggerCount; ++i) {
        struct Trigger* trigger = &gCurrentLevel->triggers[i];
        u64 cutsceneMask = 1LL << i;
        if (!(gTriggeredCutscenes & cutsceneMask) && box3DContainsPoint(&trigger->box, playerPos)) {
            cutsceneStart(&gCurrentLevel->cutscenes[trigger->cutsceneIndex]);
            // prevent the trigger from happening again
            gTriggeredCutscenes |= cutsceneMask;
        }
    }
}

struct Location* levelGetLocation(short index) {
    if (index < 0 || index >= gCurrentLevel->locationCount) {
        return NULL;
    }

    return &gCurrentLevel->locations[index];
}
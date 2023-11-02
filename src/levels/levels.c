#include "levels.h"

#include "../build/assets/test_chambers/level_list.h"
#include "../build/assets/materials/static.h"

#include "physics/collision_scene.h"
#include "static_render.h"
#include "cutscene_runner.h"
#include "../graphics/graphics.h"
#include "../player/player.h"
#include "../savefile/checkpoint.h"

#include "../util/rom.h"
#include "../util/memory.h"

struct LevelDefinition* gCurrentLevel;
int gCurrentLevelIndex;

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
    result->signalToStaticRanges = ADJUST_POINTER_POS(result->signalToStaticRanges, pointerOffset);
    result->signalToStaticIndices = ADJUST_POINTER_POS(result->signalToStaticIndices, pointerOffset);
    result->portalSurfaces = ADJUST_POINTER_POS(result->portalSurfaces, pointerOffset);

    for (int i = 0; i < result->portalSurfaceCount; ++i) {
        result->portalSurfaces[i].vertices = ADJUST_POINTER_POS(result->portalSurfaces[i].vertices, pointerOffset);
        result->portalSurfaces[i].edges = ADJUST_POINTER_POS(result->portalSurfaces[i].edges, pointerOffset);
        result->portalSurfaces[i].gfxVertices = ADJUST_POINTER_POS(result->portalSurfaces[i].gfxVertices, pointerOffset);
    }

    result->portalSurfaceMappingRange = ADJUST_POINTER_POS(result->portalSurfaceMappingRange, pointerOffset);
    result->portalSurfaceDynamicMappingRange = ADJUST_POINTER_POS(result->portalSurfaceDynamicMappingRange, pointerOffset);
    result->portalSurfaceMappingIndices = ADJUST_POINTER_POS(result->portalSurfaceMappingIndices, pointerOffset);
    result->triggers = ADJUST_POINTER_POS(result->triggers, pointerOffset);

    for (int i = 0; i < result->triggerCount; ++i) {
        result->triggers[i].triggers = ADJUST_POINTER_POS(result->triggers[i].triggers, pointerOffset);
    }
    
    result->cutscenes = ADJUST_POINTER_POS(result->cutscenes, pointerOffset);

    for (int i = 0; i < result->cutsceneCount; ++i) {
        result->cutscenes[i].steps = ADJUST_POINTER_POS(result->cutscenes[i].steps, pointerOffset);
    }

    result->locations = ADJUST_POINTER_POS(result->locations, pointerOffset);
    result->roomBvhList = ADJUST_POINTER_POS(result->roomBvhList, pointerOffset);
    result->world.rooms = ADJUST_POINTER_POS(result->world.rooms, pointerOffset);
    result->world.doorways = ADJUST_POINTER_POS(result->world.doorways, pointerOffset);

    for (int i = 0; i < result->world.roomCount; ++i) {
        result->world.rooms[i].quadIndices = ADJUST_POINTER_POS(result->world.rooms[i].quadIndices, pointerOffset);
        result->world.rooms[i].cellContents = ADJUST_POINTER_POS(result->world.rooms[i].cellContents, pointerOffset);
        result->world.rooms[i].doorwayIndices = ADJUST_POINTER_POS(result->world.rooms[i].doorwayIndices, pointerOffset);

        result->roomBvhList[i].boxIndex = ADJUST_POINTER_POS(result->roomBvhList[i].boxIndex, pointerOffset);
        result->roomBvhList[i].animatedBoxes = ADJUST_POINTER_POS(result->roomBvhList[i].animatedBoxes, pointerOffset);
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
    result->switches = ADJUST_POINTER_POS(result->switches, pointerOffset);
    result->dynamicBoxes = ADJUST_POINTER_POS(result->dynamicBoxes, pointerOffset);
    result->ballLaunchers = ADJUST_POINTER_POS(result->ballLaunchers, pointerOffset);
    result->ballCatchers = ADJUST_POINTER_POS(result->ballCatchers, pointerOffset);
    result->clocks = ADJUST_POINTER_POS(result->clocks, pointerOffset);
    result->securityCameras = ADJUST_POINTER_POS(result->securityCameras, pointerOffset);

    result->animations = ADJUST_POINTER_POS(result->animations, pointerOffset);

    for (int i = 0; i < result->animationInfoCount; ++i) {
        result->animations[i].clips = ADJUST_POINTER_POS(result->animations[i].clips, pointerOffset);
        result->animations[i].armature.boneParentIndex = ADJUST_POINTER_POS(result->animations[i].armature.boneParentIndex, pointerOffset);
        result->animations[i].armature.pose = ADJUST_POINTER_POS(result->animations[i].armature.pose, pointerOffset);
    }

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
    gQueuedLevel = NO_QUEUED_LEVEL;

    collisionSceneInit(&gCollisionScene, gCurrentLevel->collisionQuads, gCurrentLevel->collisionQuadCount, &gCurrentLevel->world);
    soundPlayerResume();
}

void levelQueueLoad(int index, struct Transform* relativeExitTransform, struct Vector3* relativeVelocity) {
    if (index == NEXT_LEVEL) {
        gQueuedLevel = gCurrentLevelIndex + 1;

        if (gQueuedLevel == LEVEL_COUNT) {
            gQueuedLevel = MAIN_MENU;
        }
    } else {
        gQueuedLevel = index;
    }
    if (relativeExitTransform) {
        gRelativeTransform = *relativeExitTransform;
    } else {
        transformInitIdentity(&gRelativeTransform);
        gRelativeTransform.position.y = 1.0f;
    }
    if (relativeVelocity) {
        gRelativeVelocity = *relativeVelocity;
    } else {
        gRelativeVelocity = gZeroVec;
    }
    checkpointClear();
}

void levelLoadLastCheckpoint() {
    cutsceneRunnerReset();
    gQueuedLevel = gCurrentLevelIndex;
    transformInitIdentity(&gRelativeTransform);
    gRelativeTransform.position.y = PLAYER_HEAD_HEIGHT;
    gRelativeVelocity = gZeroVec;
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

struct Location* levelGetLocation(short index) {
    if (index < 0 || index >= gCurrentLevel->locationCount) {
        return NULL;
    }

    return &gCurrentLevel->locations[index];
}

int levelGetChamberNumber(int levelIndex, int roomIndex){
    switch(levelIndex){
        case 0:
            if (roomIndex <= 2)
                return 0;
            else
                return 1;
        case 1:
            if (roomIndex <= 2)
                return 2;
            else
                return 3;
        case 2:
            if (roomIndex <= 2)
                return 4;
            else
                return 5;
        case 3:
            if (roomIndex <= 2)
                return 6;
            else
                return 7;
        case 4:
            return 8;
        case 5:
            return 9;
        case 6:
            return 10;
        case 7:
            if (roomIndex <= 2)
                return 11;
            else
                return 12;
        default:
            return 0;
    }
}

int chamberNumberGetLevel(int chamberIndex) {
    switch (chamberIndex) {
        case 0:
        case 1:
            return 0;
        case 2:
        case 3:
            return 1;
        case 4:
        case 5:
            return 2;
        case 6:
        case 7:
            return 3;
        case 8:
            return 4;
        case 9:
            return 5;
        case 10:
            return 6;
        case 11:
        case 12:
            return 7;
        default:
            return 0;
    }
}

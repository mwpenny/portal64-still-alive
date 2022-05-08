#include "levels.h"

#include "../build/assets/test_chambers/level_list.h"
#include "../build/assets/materials/static.h"

#include "physics/collision_scene.h"
#include "static_render.h"

struct LevelDefinition* gCurrentLevel;

int levelCount() {
    return LEVEL_COUNT;
}

void levelLoad(int index) {
    if (index < 0 || index >= LEVEL_COUNT) {
        return;
    }

    gCurrentLevel = gLevelList[index];

    collisionSceneInit(&gCollisionScene, gCurrentLevel->collisionQuads, gCurrentLevel->collisionQuadCount);
    staticRenderInit();
}

int levelMaterialCount() {
    return STATIC_MATERIAL_COUNT;
}

Gfx* levelMaterial(int index) {
    if (index < 0 || index >= STATIC_MATERIAL_COUNT) {
        return NULL;
    }

    return static_material_list[index];
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
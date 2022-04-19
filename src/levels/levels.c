#include "levels.h"

#include "../build/assets/test_chambers/level_list.h"

#include "physics/collision_scene.h"

void levelLoadCollisionScene() {
    collisionSceneInit(&gCollisionScene, test_chamber_00_test_chamber_00_0_collision_objects, TEST_CHAMBER_00_TEST_CHAMBER_00_0_QUAD_COLLIDERS_COUNT);
}
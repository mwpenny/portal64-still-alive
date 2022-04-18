#include "levels.h"

#include "../build/assets/test_chambers/level_list.h"

struct CollisionObject* levelsGetCollisionObjects() {
    return test_chamber_00_test_chamber_00_0_collision_objects;
}

int levelsGetCollisionObjectCount() {
    return TEST_CHAMBER_00_TEST_CHAMBER_00_0_QUAD_COLLIDERS_COUNT;
}
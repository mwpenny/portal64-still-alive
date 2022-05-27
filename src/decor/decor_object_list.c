#include "decor_object_list.h"

#include "../../build/assets/models/props/cylinder_test.h"
#include "../../build/assets/materials/static.h"

#include "../physics/collision_cylinder.h"

struct CollisionCylinder gCylinderCollider = {
    0.3f,
    0.35f,
};

struct DecorObjectDefinition gDecorObjectDefinitions[] = {
    [DECOR_TYPE_CYLINDER] = {
        {
            CollisionShapeTypeCylinder,
            &gCylinderCollider,
            0.0f,
            0.6f,
            &gCollisionCylinderCallbacks,
        },
        1.0f,
        0.92f,
        &props_cylinder_test_model_gfx[0],
        PLASTIC_PLASTICWALL001A_INDEX,
    }
};

struct DecorObjectDefinition* decorObjectDefinitionForId(int id) {
    if (id < 0 || id >= sizeof(gDecorObjectDefinitions) / sizeof(*gDecorObjectDefinitions)) {
        return NULL;
    }

    return &gDecorObjectDefinitions[id];
}
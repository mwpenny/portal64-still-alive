#include "decor_object_list.h"

#include "../../build/assets/models/props/cylinder_test.h"
#include "../../build/assets/models/props/radio.h"
#include "../../build/assets/materials/static.h"

#include "../physics/collision_cylinder.h"
#include "../physics/collision_box.h"

struct Vector2 gCylinderColliderEdgeVectors[] = {
    {0.0f, 1.0f},
    {0.707f, 0.707f},
    {1.0f, 0.0f},
    {0.707f, -0.707f},
};

struct CollisionCylinder gCylinderCollider = {
    0.3f,
    0.35f,
    gCylinderColliderEdgeVectors,
    sizeof(gCylinderColliderEdgeVectors) / sizeof(*gCylinderColliderEdgeVectors),
};

struct CollisionBox gRadioCollider = {
    {0.23351f, 0.20597f, 0.154298f},
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
    },
    [DECOR_TYPE_RADIO] = {
        {
            CollisionShapeTypeBox,
            &gRadioCollider,
            0.0f,
            0.6f,
            &gCollisionBoxCallbacks,
        },
        0.2f,
        0.4f,
        &props_radio_model_gfx[0],
        RADIO_INDEX,
    },
};

struct DecorObjectDefinition* decorObjectDefinitionForId(int id) {
    if (id < 0 || id >= sizeof(gDecorObjectDefinitions) / sizeof(*gDecorObjectDefinitions)) {
        return NULL;
    }

    return &gDecorObjectDefinitions[id];
}
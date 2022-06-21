#include "decor_object_list.h"

#include "../../build/assets/models/props/cylinder_test.h"
#include "../../build/assets/models/cube/cube.h"
#include "../../build/assets/models/props/radio.h"
#include "../../build/assets/materials/static.h"
#include "../../build/src/audio/clips.h"

#include "../physics/collision_cylinder.h"
#include "../physics/collision_box.h"

struct Vector2 gCylinderColliderEdgeVectors[] = {
    {0.0f, 1.0f},
    {0.707f, 0.707f},
    {1.0f, 0.0f},
    {0.707f, -0.707f},
};

struct CollisionQuad gCyliderColliderFaces[8];

struct CollisionCylinder gCylinderCollider = {
    0.3f,
    0.35f,
    gCylinderColliderEdgeVectors,
    sizeof(gCylinderColliderEdgeVectors) / sizeof(*gCylinderColliderEdgeVectors),
    gCyliderColliderFaces,
};

struct CollisionBox gRadioCollider = {
    {0.23351f, 0.20597f, 0.154298f},
};

struct CollisionBox gCubeCollisionBox = {
    {0.3165f, 0.3165f, 0.3165f}
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
        .materialIndex = PLASTIC_PLASTICWALL001A_INDEX,
        .soundClipId = -1,
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
        .materialIndex = RADIO_INDEX,
        .soundClipId = SOUNDS_LOOPING_RADIO_MIX,
    },
    [DECOR_TYPE_CUBE] = {
        {
            CollisionShapeTypeBox,
            &gCubeCollisionBox,
            0.0f,
            0.5f,
            &gCollisionBoxCallbacks,  
        },
        2.0f, 
        0.55f,
        &cube_cube_model_gfx[0],
        .materialIndex = CUBE_INDEX,
        .materialIndexFizzled = CUBE_FIZZLED_INDEX,
        .soundClipId = -1,
    },
};

struct DecorObjectDefinition* decorObjectDefinitionForId(int id) {
    if (id < 0 || id >= sizeof(gDecorObjectDefinitions) / sizeof(*gDecorObjectDefinitions)) {
        return NULL;
    }

    return &gDecorObjectDefinitions[id];
}
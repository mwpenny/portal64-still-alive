#include "decor_object_list.h"

#include "../../build/assets/materials/static.h"
#include "../../build/src/audio/clips.h"
#include "../../build/assets/models/dynamic_model_list.h"

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
        PROPS_CYLINDER_TEST_DYNAMIC_MODEL,
        .materialIndex = PLASTIC_PLASTICWALL001A_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
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
        PROPS_RADIO_DYNAMIC_MODEL,
        .materialIndex = RADIO_INDEX,
        .materialIndexFizzled = RADIO_FIZZLED_INDEX,
        .soundClipId = SOUNDS_LOOPING_RADIO_MIX,
        .soundFizzleId = SOUNDS_DINOSAUR_FIZZLE,
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
        CUBE_CUBE_DYNAMIC_MODEL,
        .materialIndex = CUBE_INDEX,
        .materialIndexFizzled = CUBE_FIZZLED_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
        .flags = DecorObjectFlagsImportant,
    },
    [DECOR_TYPE_CUBE_UNIMPORTANT] = {
        {
            CollisionShapeTypeBox,
            &gCubeCollisionBox,
            0.0f,
            0.5f,
            &gCollisionBoxCallbacks,  
        },
        2.0f, 
        0.55f,
        CUBE_CUBE_DYNAMIC_MODEL,
        .materialIndex = CUBE_INDEX,
        .materialIndexFizzled = CUBE_FIZZLED_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_AUTOPORTAL_FRAME] = {
        {
            CollisionShapeTypeNone,
            NULL,
            0.0f,
            0.0f,
            NULL,  
        },
        0.0f, 
        1.0f,
        PROPS_AUTOPORTAL_FRAME_AUTOPORTAL_FRAME_DYNAMIC_MODEL,
        .materialIndex = AUTOPORTAL_FRAME_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    }
};

struct DecorObjectDefinition* decorObjectDefinitionForId(int id) {
    if (id < 0 || id >= sizeof(gDecorObjectDefinitions) / sizeof(*gDecorObjectDefinitions)) {
        return NULL;
    }

    return &gDecorObjectDefinitions[id];
}

int decorIdForObjectDefinition(struct DecorObjectDefinition* def) {
    int result = def - gDecorObjectDefinitions;

    if (result < 0 || result >= sizeof(gDecorObjectDefinitions) / sizeof(*gDecorObjectDefinitions)) {
        return -1;
    }

    return result;
}
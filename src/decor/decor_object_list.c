#include "decor_object_list.h"

#include "physics/collision_cylinder.h"
#include "physics/collision_box.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_model_list.h"
#include "codegen/assets/models/props/light_rail_endcap.h"

struct Vector2 gCylinderColliderEdgeVectors[] = {
    {0.0f, 1.0f},
    {0.707f, 0.707f},
    {1.0f, 0.0f},
    {0.707f, -0.707f},
};

struct CollisionQuad gCylinderColliderFaces[8];
struct CollisionCylinder gCylinderCollider = {
    0.3f,
    0.35f,
    gCylinderColliderEdgeVectors,
    sizeof(gCylinderColliderEdgeVectors) / sizeof(*gCylinderColliderEdgeVectors),
    gCylinderColliderFaces,
};

struct CollisionBox gRadioCollider = {
    {0.23351f, 0.20597f, 0.154298f},
};

struct CollisionBox gCubeCollisionBox = {
    {0.3165f, 0.3165f, 0.3165f}
};

struct CollisionQuad gFoodCanColliderFaces[8];
struct CollisionCylinder gFoodCanCollider = {
    0.11f,
    0.125f,
    gCylinderColliderEdgeVectors,
    sizeof(gCylinderColliderEdgeVectors) / sizeof(*gCylinderColliderEdgeVectors),
    gFoodCanColliderFaces,
};

struct CollisionQuad gWaterBottleColliderFaces[8];
struct CollisionCylinder gWaterBottleCollider = {
    0.24f,
    0.25f,
    gCylinderColliderEdgeVectors,
    sizeof(gCylinderColliderEdgeVectors) / sizeof(*gCylinderColliderEdgeVectors),
    gWaterBottleColliderFaces,
};

struct CollisionBox gSaucepanCollisionBox = {
    {0.15f, 0.15f, 0.1f}
};

struct CollisionQuad gBucketColliderFaces[8];
struct CollisionCylinder gBucketCollider = {
    0.2f,
    0.2f,
    gCylinderColliderEdgeVectors,
    sizeof(gCylinderColliderEdgeVectors) / sizeof(*gCylinderColliderEdgeVectors),
    gBucketColliderFaces,
};

struct CollisionBox gMilkCartonCollisionBox = {
    {0.125f, 0.125f, 0.2f}
};

struct CollisionBox gPCCaseCollisionBox = {
    {0.25f, 0.25f, 0.15f}
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
        2.5f,
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
        2.5f,
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
    },
    [DECOR_TYPE_LIGHT_RAIL_ENDCAP] = {
        {
            CollisionShapeTypeNone,
            NULL,
            0.0f,
            0.0f,
            NULL,  
        },
        0.0f, 
        1.3f,
        PROPS_LIGHT_RAIL_ENDCAP_DYNAMIC_MODEL,
        .materialIndex = LIGHT_RAIL_ENDCAP_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_LAB_MONITOR] = {
        {
            CollisionShapeTypeNone,
            NULL,
            0.0f,
            0.0f,
            NULL,  
        },
        0.0f, 
        1.0f,
        PROPS_LAB_MONITOR_DYNAMIC_MODEL,
        .materialIndex = LAB_MONITOR_SCREEN_TEXT_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_LAB_CHAIR] = {
        {
            CollisionShapeTypeNone,
            NULL,
            0.0f,
            0.0f,
            NULL,  
        },
        0.0f, 
        1.0f,
        PROPS_LAB_CHAIR_DYNAMIC_MODEL,
        .materialIndex = DEFAULT_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_LAB_DESK01] = {
        {
            CollisionShapeTypeNone,
            NULL,
            0.0f,
            0.0f,
            NULL,  
        },
        0.0f, 
        1.0f,
        PROPS_LAB_DESK_LAB_DESK01_DYNAMIC_MODEL,
        .materialIndex = DEFAULT_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_LAB_DESK02] = {
        {
            CollisionShapeTypeNone,
            NULL,
            0.0f,
            0.0f,
            NULL,  
        },
        0.0f, 
        1.0f,
        PROPS_LAB_DESK_LAB_DESK02_DYNAMIC_MODEL,
        .materialIndex = DEFAULT_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_LAB_DESK03] = {
        {
            CollisionShapeTypeNone,
            NULL,
            0.0f,
            0.0f,
            NULL,  
        },
        0.0f, 
        1.0f,
        PROPS_LAB_DESK_LAB_DESK03_DYNAMIC_MODEL,
        .materialIndex = DEFAULT_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_LAB_DESK04] = {
        {
            CollisionShapeTypeNone,
            NULL,
            0.0f,
            0.0f,
            NULL,  
        },
        0.0f, 
        1.0f,
        PROPS_LAB_DESK_LAB_DESK04_DYNAMIC_MODEL,
        .materialIndex = DEFAULT_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_SCRAWLINGS002A] = {
        {
            CollisionShapeTypeNone,
            NULL,
            0.0f,
            0.0f,
            NULL,
        },
        0.0f,
        1.5f,
        OVERLAYS_OVERLAY_SCRAWLINGS002A_DYNAMIC_MODEL,
        .materialIndex = OVERLAY_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_FOOD_CAN] = {
        {
            CollisionShapeTypeCylinder,
            &gFoodCanCollider,
            0.0f,
            0.4f,
            &gCollisionCylinderCallbacks,
        },
        0.8f,
        0.18f,
        PROPS_FOOD_CAN_FOOD_CAN_DYNAMIC_MODEL,
        .materialIndex = FOOD_CAN_INDEX,
        .materialIndexFizzled = FOOD_CAN_FIZZLED_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_WATER_BOTTLE] = {
        {
            CollisionShapeTypeCylinder,
            &gWaterBottleCollider,
            0.0f,
            0.75f,
            &gCollisionCylinderCallbacks,
        },
        1.0f,
        0.4f,
        PROPS_WATER_BOTTLE_WATER_BOTTLE_DYNAMIC_MODEL,
        .materialIndex = LIT_OBJECT_INDEX,
        .materialIndexFizzled = LIT_OBJECT_FIZZLED_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_SAUCEPAN] = {
        {
            CollisionShapeTypeBox,
            &gSaucepanCollisionBox,
            0.0f,
            0.5f,
            &gCollisionBoxCallbacks,
        },
        1.25f,
        0.375f,
        PROPS_SAUCEPAN_SAUCEPAN_DYNAMIC_MODEL,
        .materialIndex = SAUCEPAN_INDEX,
        .materialIndexFizzled = SAUCEPAN_FIZZLED_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_METALBUCKET01A] = {
        {
            CollisionShapeTypeCylinder,
            &gBucketCollider,
            0.0f,
            0.75f,
            &gCollisionCylinderCallbacks,
        },
        1.25f,
        0.4f,
        PROPS_JUNK_METALBUCKET01A_DYNAMIC_MODEL,
        .materialIndex = METALBUCKET01A_INDEX,
        .materialIndexFizzled = METALBUCKET01A_FIZZLED_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_MILK_CARTON] = {
        {
            CollisionShapeTypeBox,
            &gMilkCartonCollisionBox,
            0.0f,
            0.4f,
            &gCollisionBoxCallbacks,
        },
        0.8f,
        0.4f,
        PROPS_MILK_CARTON_MILK_CARTON_DYNAMIC_MODEL,
        .materialIndex = MILK_CARTON_INDEX,
        .materialIndexFizzled = MILK_CARTON_FIZZLED_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_MILK_CARTON_OPEN] = {
        {
            CollisionShapeTypeBox,
            &gMilkCartonCollisionBox,
            0.0f,
            0.4f,
            &gCollisionBoxCallbacks,
        },
        0.8f,
        0.4f,
        PROPS_MILK_CARTON_MILK_CARTON_OPEN_DYNAMIC_MODEL,
        .materialIndex = MILK_CARTON_INDEX,
        .materialIndexFizzled = MILK_CARTON_FIZZLED_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
    [DECOR_TYPE_PC_CASE_OPEN] = {
        {
            CollisionShapeTypeBox,
            &gPCCaseCollisionBox,
            0.0f,
            0.5f,
            &gCollisionBoxCallbacks,
        },
        1.5f,
        0.25f,
        PROPS_PC_CASE_OPEN_PC_CASE_OPEN_DYNAMIC_MODEL,
        .materialIndex = LIT_OBJECT_INDEX,
        .materialIndexFizzled = LIT_OBJECT_FIZZLED_INDEX,
        .soundClipId = -1,
        .soundFizzleId = -1,
    },
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

int decorIdForCollisionObject(struct CollisionObject* collisionObject) {
    return decorIdForObjectDefinition((struct DecorObjectDefinition*)collisionObject->collider);
}

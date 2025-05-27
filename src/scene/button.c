#include "button.h"

#include "defs.h"
#include "dynamic_scene.h"
#include "graphics/renderstate.h"
#include "physics/collision_cylinder.h"
#include "physics/collision_scene.h"
#include "physics/contact_solver.h"
#include "scene/hud.h"
#include "scene/scene.h"
#include "system/time.h"
#include "util/dynamic_asset_loader.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/props/button.h"

struct Vector2 gButtonCylinderEdgeVectors[] = {
    {0.0f, 1.0f},
    {0.707f, 0.707f},
    {1.0f, 0.0f},
    {0.707f, -0.707f},
};

struct CollisionQuad gButtonCylinderFaces[8];

struct CollisionCylinder gButtonCylinder = {
    0.5f,
    0.3f,
    gButtonCylinderEdgeVectors,
    sizeof(gButtonCylinderEdgeVectors) / sizeof(*gButtonCylinderEdgeVectors),
    gButtonCylinderFaces,
};

struct ColliderTypeData gButtonCollider = {
    CollisionShapeTypeCylinder,
    &gButtonCylinder,
    0.0f,
    1.0f,
    &gCollisionCylinderCallbacks
};

#define MASS_BUTTON_PRESS_THRESHOLD     1.9f
#define BUTTON_MOVEMENT_AMOUNT          0.1f
#define BUTTON_MOVE_VELOCTY             0.3f

#define PRESSED_WITH_CUBE               2

#define CUBE_PRESS_IDLE_FRAMES          2

void buttonRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Button* button = (struct Button*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    guTranslate(matrix, button->originalPos.x * SCENE_SCALE, button->originalPos.y * SCENE_SCALE, button->originalPos.z * SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, PROPS_BUTTON_DEFAULT_BONES_COUNT);

    if (!armature) {
        return;
    }

    struct SKArmatureWithAnimations* armatureDef = dynamicAssetAnimatedModel(PROPS_BUTTON_DYNAMIC_ANIMATED_MODEL);

    transformToMatrixL(&armatureDef->armature->pose[PROPS_BUTTON_BUTTONBASE_BONE], &armature[PROPS_BUTTON_BUTTONBASE_BONE], 1.0f);

    // reusing global memory
    armatureDef->armature->pose[PROPS_BUTTON_BUTTONPAD_BONE].position.y = (button->rigidBody.transform.position.y - button->originalPos.y) * SCENE_SCALE;
    transformToMatrixL(&armatureDef->armature->pose[PROPS_BUTTON_BUTTONPAD_BONE], &armature[PROPS_BUTTON_BUTTONPAD_BONE], 1.0f);

    dynamicRenderListAddData(renderList, armatureDef->armature->displayList, matrix, BUTTON_INDEX, &button->rigidBody.transform.position, armature);
}

void buttonInit(struct Button* button, struct ButtonDefinition* definition) {
    dynamicAssetAnimatedModel(PROPS_BUTTON_DYNAMIC_ANIMATED_MODEL);

    collisionObjectInit(&button->collisionObject, &gButtonCollider, &button->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_BLOCK_PORTAL);
    rigidBodyMarkKinematic(&button->rigidBody);
    collisionSceneAddDynamicObject(&button->collisionObject);

    button->rigidBody.transform.position = definition->location;
    quatIdent(&button->rigidBody.transform.rotation);
    button->rigidBody.transform.scale = gOneVec;
    button->rigidBody.currentRoom = definition->roomIndex;

    collisionObjectUpdateBB(&button->collisionObject);

    button->dynamicId = dynamicSceneAdd(button, buttonRender, &button->rigidBody.transform.position, 0.84f);
    button->signalIndex = definition->signalIndex;

    button->originalPos = definition->location;
    button->cubeSignalIndex = definition->cubeSignalIndex;
    button->flags = 0;
    
    button->cubePressFrames = CUBE_PRESS_IDLE_FRAMES;

    dynamicSceneSetRoomFlags(button->dynamicId, ROOM_FLAG_FROM_INDEX(button->rigidBody.currentRoom));
}

void buttonUpdate(struct Button* button) {
    struct ContactManifold* manifold = contactSolverNextManifold(&gContactSolver, &button->collisionObject, NULL);

    int shouldPress = 0;
    while (manifold) {
        struct CollisionObject* other = manifold->shapeA == &button->collisionObject ? manifold->shapeB : manifold->shapeA;

        if (other->body && other->body->mass > MASS_BUTTON_PRESS_THRESHOLD) {
            
            shouldPress = 1;

            if ((other->body->flags & RigidBodyFlagsGrabbable) == RigidBodyFlagsGrabbable && button->cubePressFrames <= 0) {
                shouldPress = PRESSED_WITH_CUBE;
            }
            
            break;
        }

        manifold = contactSolverNextManifold(&gContactSolver, &button->collisionObject, manifold);
    }
    
    if (button->collisionObject.flags & COLLISION_OBJECT_PLAYER_STANDING) {
        button->collisionObject.flags &= ~COLLISION_OBJECT_PLAYER_STANDING;
        shouldPress = 1;
    }

    struct Vector3 targetPos = button->originalPos;
    
    if (shouldPress) {
        targetPos.y -= BUTTON_MOVEMENT_AMOUNT;
        signalsSend(button->signalIndex);

        if (button->cubeSignalIndex != -1 && shouldPress == PRESSED_WITH_CUBE) {
            signalsSend(button->cubeSignalIndex);
        }
        
        if (button->cubePressFrames > 0) {
            --button->cubePressFrames;
        }
    } else {
        button->cubePressFrames = CUBE_PRESS_IDLE_FRAMES;
    }

    //if its actively moving up or down
    if (targetPos.y != button->rigidBody.transform.position.y) {
        //actively going down
        if (shouldPress){
            if (!(button->flags & ButtonFlagsBeingPressed)){
                soundPlayerPlay(soundsButton, 2.5f, 0.5f, &button->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
                hudShowSubtitle(&gScene.hud, PORTAL_BUTTON_DOWN, SubtitleTypeCaption);
            }
            button->flags |= ButtonFlagsBeingPressed;
        }
        // actively going up
        else{
            if ((button->flags & ButtonFlagsBeingPressed)){
                soundPlayerPlay(soundsButtonRelease, 2.5f, 0.4f, &button->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
                hudShowSubtitle(&gScene.hud, PORTAL_BUTTON_UP, SubtitleTypeCaption);
            }
            button->flags &= ~ButtonFlagsBeingPressed;
        }

        vector3MoveTowards(&button->rigidBody.transform.position, &targetPos, BUTTON_MOVE_VELOCTY * FIXED_DELTA_TIME, &button->rigidBody.transform.position);
        collisionObjectUpdateBB(&button->collisionObject);
    }
    
}
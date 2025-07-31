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

#define OBJECT_PRESS_DELAY_FRAMES       2

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

static void buttonHandleCollideStartEnd(struct CollisionObject* object, struct CollisionObject* other, struct Vector3* normal) {
    if (other->body == NULL || (other->body->flags & RigidBodyIsKinematic)) {
        return;
    }

    struct Button* button = object->data;

    if (normal != NULL) {
        // Ensure rigid bodies which stay asleep on load will hit the button
        // Keeps activating bodies activating, and non-activating bodies on top
        other->body->flags |= RigidBodyForceWakeOnLoad;
    } else {
        other->body->flags &= ~RigidBodyForceWakeOnLoad;
    }

    if ((other->body->flags & RigidBodyFlagsGrabbable) && other->body->mass > MASS_BUTTON_PRESS_THRESHOLD) {
        button->activatingObjectCount += (normal != NULL) ? 1 : -1;
    }
}

void buttonInit(struct Button* button, struct ButtonDefinition* definition) {
    dynamicAssetAnimatedModel(PROPS_BUTTON_DYNAMIC_ANIMATED_MODEL);

    collisionObjectInit(&button->collisionObject, &gButtonCollider, &button->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_BLOCK_PORTAL);
    button->collisionObject.collideStartEnd = buttonHandleCollideStartEnd;
    button->collisionObject.data = button;

    rigidBodyMarkKinematic(&button->rigidBody);
    collisionSceneAddDynamicObject(&button->collisionObject);

    button->rigidBody.transform.position = definition->location;
    quatIdent(&button->rigidBody.transform.rotation);
    button->rigidBody.transform.scale = gOneVec;
    button->rigidBody.currentRoom = definition->roomIndex;

    collisionObjectUpdateBB(&button->collisionObject);

    button->originalPos = definition->location;
    button->flags = ButtonFlagsFirstUpdate;
    button->state = ButtonStateUnpressed;

    button->dynamicId = dynamicSceneAdd(button, buttonRender, &button->rigidBody.transform.position, 0.84f);
    dynamicSceneSetRoomFlags(button->dynamicId, ROOM_FLAG_FROM_INDEX(button->rigidBody.currentRoom));

    button->signalIndex = definition->signalIndex;
    button->objectSignalIndex = definition->objectSignalIndex;
    button->objectPressTimer = OBJECT_PRESS_DELAY_FRAMES;

    button->activatingObjectCount = 0;
}

void buttonUpdate(struct Button* button) {
    if (button->state != ButtonStateUnpressed) {
        signalsSend(button->signalIndex);

        if (button->objectSignalIndex != -1 && button->state == ButtonStatePressedByObject) {
            signalsSend(button->objectSignalIndex);
        }
    }

    if (button->flags & ButtonFlagsFirstUpdate) {
        // Objects are updated before collision is processed, so the first
        // update after loading a save will not see objects on the button.
        //
        // Skip checking this during the first update to avoid bouncing.
        button->flags &= ~ButtonFlagsFirstUpdate;
        return;
    }

    enum ButtonState newState = ButtonStateUnpressed;

    // Check for objects on button
    if (button->collisionObject.flags & COLLISION_OBJECT_PLAYER_STANDING) {
        button->collisionObject.flags &= ~COLLISION_OBJECT_PLAYER_STANDING;
        newState = ButtonStatePressed;
    } else if (button->activatingObjectCount > 0) {
        if (button->objectPressTimer == 0) {
            newState = ButtonStatePressedByObject;
        } else {
            newState = ButtonStatePressed;
        }
    }

    struct Vector3 targetPos = button->originalPos;
    if (newState != ButtonStateUnpressed) {
        targetPos.y -= BUTTON_MOVEMENT_AMOUNT;

        if (button->objectPressTimer > 0) {
            --button->objectPressTimer;
        }
    } else {
        button->objectPressTimer = OBJECT_PRESS_DELAY_FRAMES;
    }

    // Update state
    if (button->state != newState) {
        if (newState == ButtonStateUnpressed) {
            soundPlayerPlay(soundsButtonRelease, 2.5f, 1.0f, &button->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
            hudShowSubtitle(&gScene.hud, PORTAL_BUTTON_UP, SubtitleTypeCaption);
        } else if (button->state == ButtonStateUnpressed) {
            soundPlayerPlay(soundsButton, 2.5f, 1.0f, &button->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
            hudShowSubtitle(&gScene.hud, PORTAL_BUTTON_DOWN, SubtitleTypeCaption);
        }

        button->state = newState;
    }

    // Adjust button position based on state
    if (targetPos.y != button->rigidBody.transform.position.y) {
        vector3MoveTowards(
            &button->rigidBody.transform.position,
            &targetPos,
            BUTTON_MOVE_VELOCTY * FIXED_DELTA_TIME,
            &button->rigidBody.transform.position
        );
        collisionObjectUpdateBB(&button->collisionObject);
    }
}

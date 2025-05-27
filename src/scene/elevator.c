#include "elevator.h"

#include "audio/clips.h"
#include "audio/soundplayer.h"
#include "controls/rumble_pak.h"
#include "math/mathf.h"
#include "physics/collision_scene.h"
#include "physics/mesh_collider.h"
#include "savefile/checkpoint.h"
#include "scene/dynamic_scene.h"
#include "signals.h"
#include "system/time.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/props/round_elevator_collision.h"
#include "codegen/assets/models/props/round_elevator_interior.h"
#include "codegen/assets/models/props/round_elevator.h"

#define AUTO_OPEN_DISTANCE      4.0f
#define INSIDE_DISTANCE         1.2f
#define SAME_LEVEL_HEIGHT       3.0f
#define OPEN_SPEED              2.0f

#define OPEN_DELAY              1.0f
#define CLOSE_DELAY             10.0f
#define MOVING_SOUND_DELAY      1.5f  
#define SHAKE_DURATION          0.5f    

struct ColliderTypeData gElevatorColliderType = {
    CollisionShapeTypeMesh,
    &props_round_elevator_collision_collider,
    0.0f, 0.6f,
    &gMeshColliderCallbacks
};

int gElevatorCollisionLayers = COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_BALL | COLLISION_LAYERS_TANGIBLE;

struct Vector3 gClosedPosition[] = {
    [PROPS_ROUND_ELEVATOR_ELEVATOR_BONE] = {0, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORLEFT_BONE] = {1.46439 * SCENE_SCALE, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORRIGHT_BONE] = {1.46439 * SCENE_SCALE, 0, 0},
};

struct Vector3 gOpenPosition[] = {
    [PROPS_ROUND_ELEVATOR_ELEVATOR_BONE] = {0, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORLEFT_BONE] = {1.188716 * SCENE_SCALE, 0.0f, 0.653916 * SCENE_SCALE},
    [PROPS_ROUND_ELEVATOR_DOORRIGHT_BONE] = {1.188716 * SCENE_SCALE, 0.0f, -0.653916 * SCENE_SCALE},
};

void elevatorRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Elevator* elevator = (struct Elevator*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);
    transformToMatrixL(&elevator->rigidBody.transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, PROPS_ROUND_ELEVATOR_DEFAULT_BONES_COUNT);

    if (!armature) {
        return;
    }

    for (int i = 0; i < PROPS_ROUND_ELEVATOR_DEFAULT_BONES_COUNT; ++i) {
        vector3Lerp(&gClosedPosition[i], &gOpenPosition[i], elevator->openAmount, &props_round_elevator_default_bones[i].position);
        transformToMatrixL(&props_round_elevator_default_bones[i], &armature[i], 1.0f);
    }

    dynamicRenderListAddData(
        renderList, 
        props_round_elevator_model_gfx, 
        matrix, 
        DEFAULT_INDEX, 
        &elevator->rigidBody.transform.position, 
        armature
    );

    int insideCheck = elevator->flags & (ElevatorFlagsIsLocked | ElevatorFlagsIsExit);

    int isPlayerInside = insideCheck == ElevatorFlagsIsLocked || insideCheck == ElevatorFlagsIsExit;

    if (elevator->openAmount > 0.0f || isPlayerInside) {
        dynamicRenderListAddData(
            renderList, 
            props_round_elevator_interior_model_gfx, 
            matrix, 
            DEFAULT_INDEX, 
            &elevator->rigidBody.transform.position, 
            NULL
        );  
    }
}

void elevatorInit(struct Elevator* elevator, struct ElevatorDefinition* elevatorDefinition) {
    collisionObjectInit(&elevator->collisionObject, &gElevatorColliderType, &elevator->rigidBody, 1.0f, gElevatorCollisionLayers);
    rigidBodyMarkKinematic(&elevator->rigidBody);
    collisionSceneAddDynamicObject(&elevator->collisionObject);

    elevator->rigidBody.transform.position = elevatorDefinition->position;
    elevator->rigidBody.transform.rotation = elevatorDefinition->rotation;
    elevator->rigidBody.currentRoom = elevatorDefinition->roomIndex;

    collisionObjectUpdateBB(&elevator->collisionObject);

    elevator->dynamicId = dynamicSceneAdd(elevator, elevatorRender, &elevator->rigidBody.transform.position, 3.9f);
    elevator->flags = elevatorDefinition->targetElevator == -1 ? ElevatorFlagsIsExit : 0;
    elevator->openAmount = 0.0f;
    elevator->targetElevator = elevatorDefinition->targetElevator;

    elevator->timer = elevatorDefinition->targetElevator == -1 ? OPEN_DELAY : CLOSE_DELAY;
    elevator->movingTimer = MOVING_SOUND_DELAY;
    
    dynamicSceneSetRoomFlags(elevator->dynamicId, ROOM_FLAG_FROM_INDEX(elevatorDefinition->roomIndex));
}


unsigned char gElevatorRumbleData[] = {
    0xEF, 0xE9, 0xAA, 0xAA, 0xAA, 0x55
};

struct RumblePakWave gElevatorRumbleWave = {
    .samples = gElevatorRumbleData,
    .sampleCount = 24,
    .samplesPerTick = 1 << 4,
};

int elevatorUpdate(struct Elevator* elevator, struct Player* player) {
    struct Vector3 offset;
    vector3Sub(&elevator->rigidBody.transform.position, &player->lookTransform.position, &offset);
    
    float verticalDistance = fabsf(offset.y);
    offset.y = 0.0f;

    float horizontalDistance = vector3MagSqrd(&offset);

    int inRange = horizontalDistance < AUTO_OPEN_DISTANCE * AUTO_OPEN_DISTANCE && verticalDistance < SAME_LEVEL_HEIGHT;
    int inside = horizontalDistance < INSIDE_DISTANCE * INSIDE_DISTANCE && verticalDistance < SAME_LEVEL_HEIGHT;
    int cutscenePreventingMovement = ((gScene.boolCutsceneIsRunning==1) && (elevator->targetElevator >= gScene.elevatorCount));

    int shouldBeOpen;
    int shouldLock;

    short result = -1;

    if ((elevator->flags & ElevatorFlagsIsExit)) {
        if (inside && !cutscenePreventingMovement) {
            elevator->timer -= FIXED_DELTA_TIME;
        }

        shouldBeOpen = elevator->timer < 0.0f;
        shouldLock = !inRange && (elevator->flags & ElevatorFlagsHasHadPlayer) != 0;
    } else {
        shouldBeOpen = inRange && !inside;
        shouldLock = inside;
        
        if ((inside || (elevator->flags & ElevatorFlagsIsLocked) != 0) && !cutscenePreventingMovement) {
            elevator->timer -= FIXED_DELTA_TIME;

            if (elevator->timer < 0.0f) {
                elevator->flags &= ~ElevatorFlagsIsLocked;
                shouldLock = 0;
                result = elevator->targetElevator;
            }
        }
    }

    if (inside) {
        elevator->flags |= ElevatorFlagsHasHadPlayer;
    }

    if (shouldLock) {
        int shouldSaveCheckpoint = (elevator->flags & (ElevatorFlagsIsLocked | ElevatorFlagsIsExit)) == ElevatorFlagsIsExit;

        elevator->flags |= ElevatorFlagsIsLocked;   

        if (shouldSaveCheckpoint) {
            // save the checkpoint after flag ElevatorFlagsIsLocked is set
            // so loading this checkpoint doesn't immediately create another
            // save checkpoint
            sceneQueueCheckpoint(&gScene);
        }
    }

    if ((elevator->flags & ElevatorFlagsIsLocked) != 0) {
        shouldBeOpen = 0;
    }

    if (inRange) {
        // since this logic is modifying a single copy of the collider it should only update the collision layers for the elevator close to the player
        if (shouldBeOpen) {
            props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_RIGHT_COLLISION_INDEX].collisionLayers = 0;
            props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_LEFT_COLLISION_INDEX].collisionLayers = 0;
        } else {
            props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_RIGHT_COLLISION_INDEX].collisionLayers = gElevatorCollisionLayers;
            props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_LEFT_COLLISION_INDEX].collisionLayers = gElevatorCollisionLayers;
        }
    }

    int isClosed = elevator->openAmount == 0.0f;
    int isOpen = elevator->openAmount == 1.0f;

    if ((isClosed && shouldBeOpen) || (isOpen && !shouldBeOpen)) {
        soundPlayerPlay(soundsElevatorDoor, 1.0f, 0.5f, &elevator->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
        if ((isClosed && shouldBeOpen) && (elevator->flags & ElevatorFlagsHasHadPlayer)){
            soundPlayerPlay(soundsElevatorChime, 1.5f, 0.5f, &elevator->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
            hudShowSubtitle(&gScene.hud, PORTAL_ELEVATOR_CHIME, SubtitleTypeCaption);
        }
    }
    

    if ((elevator->flags & ElevatorFlagsIsLocked) && isClosed && (elevator->movingTimer > 0.0f) && !cutscenePreventingMovement){
        elevator->movingTimer -= FIXED_DELTA_TIME;
    }

    if ((elevator->flags & ElevatorFlagsIsLocked) && 
        isClosed && 
        !(elevator->flags & ElevatorFlagsMovingSoundPlayed) && 
        (elevator->movingTimer <= 0.0f) && 
        !cutscenePreventingMovement &&
        elevator->targetElevator >= 0) {

        soundPlayerPlay(soundsElevatorMoving, 1.25f, 0.5f, &elevator->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
        hudShowSubtitle(&gScene.hud, PORTAL_ELEVATOR_START, SubtitleTypeCaption);
        player->shakeTimer = SHAKE_DURATION;
        rumblePakClipPlay(&gElevatorRumbleWave);
        elevator->flags |= ElevatorFlagsMovingSoundPlayed;
    }

    elevator->openAmount = mathfMoveTowards(elevator->openAmount, shouldBeOpen ? 1.0f : 0.0f, OPEN_SPEED * FIXED_DELTA_TIME);

    return result;
}
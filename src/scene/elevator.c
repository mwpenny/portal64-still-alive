#include "elevator.h"

#include "audio/clips.h"
#include "audio/soundplayer.h"
#include "controls/rumble_pak.h"
#include "levels/cutscene_runner.h"
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

#define DOOR_SPEED              2.0f

#define OPEN_DELAY              1.0f
#define TELEPORT_DELAY          10.0f
#define MOVE_SOUND_DELAY        2.5f
#define MOVE_SOUND_START        (TELEPORT_DELAY - MOVE_SOUND_DELAY)
#define MOVE_SHAKE_DURATION     0.5f

static unsigned char sElevatorRumbleData[] = {
    0xEF, 0xE9, 0xAA, 0xAA, 0xAA, 0x55
};
static struct RumblePakWave sElevatorRumbleWave = {
    .samples = sElevatorRumbleData,
    .sampleCount = 24,
    .samplesPerTick = 1 << 4,
};

static struct ColliderTypeData sElevatorColliderType = {
    CollisionShapeTypeMesh,
    &props_round_elevator_collision_collider,
    0.0f, 0.6f,
    &gMeshColliderCallbacks
};

static int sElevatorCollisionLayers = COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_BALL | COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_BLOCK_TURRET_SIGHT;

static struct Vector3 sClosedPosition[] = {
    [PROPS_ROUND_ELEVATOR_ELEVATOR_BONE] = {0, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORLEFT_BONE] = {1.46439 * SCENE_SCALE, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORRIGHT_BONE] = {1.46439 * SCENE_SCALE, 0, 0},
};

static struct Vector3 sOpenPosition[] = {
    [PROPS_ROUND_ELEVATOR_ELEVATOR_BONE] = {0, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORLEFT_BONE] = {1.188716 * SCENE_SCALE, 0.0f, 0.653916 * SCENE_SCALE},
    [PROPS_ROUND_ELEVATOR_DOORRIGHT_BONE] = {1.188716 * SCENE_SCALE, 0.0f, -0.653916 * SCENE_SCALE},
};

void elevatorRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Elevator* elevator = (struct Elevator*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);
    if (!matrix) {
        return;
    }

    transformToMatrixL(&elevator->rigidBody.transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, PROPS_ROUND_ELEVATOR_DEFAULT_BONES_COUNT);
    if (!armature) {
        return;
    }

    for (int i = 0; i < PROPS_ROUND_ELEVATOR_DEFAULT_BONES_COUNT; ++i) {
        vector3Lerp(&sClosedPosition[i], &sOpenPosition[i], elevator->openAmount, &props_round_elevator_default_bones[i].position);
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

    // The current function will only be called if the elevator is visible
    // Closed, unlocked arrival elevators can only be seen when teleported into
    // Departure elevators only close and lock once inside
    int insideCheck = elevator->flags & (ElevatorFlagsIsLocked | ElevatorFlagsIsArrival);
    int isPlayerInside = insideCheck == ElevatorFlagsIsLocked || insideCheck == ElevatorFlagsIsArrival;

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
    collisionObjectInit(&elevator->collisionObject, &sElevatorColliderType, &elevator->rigidBody, 1.0f, sElevatorCollisionLayers);
    rigidBodyMarkKinematic(&elevator->rigidBody);
    collisionSceneAddDynamicObject(&elevator->collisionObject);

    elevator->rigidBody.transform.position = elevatorDefinition->position;
    elevator->rigidBody.transform.rotation = elevatorDefinition->rotation;
    elevator->rigidBody.currentRoom = elevatorDefinition->roomIndex;

    collisionObjectUpdateBB(&elevator->collisionObject);

    elevator->dynamicId = dynamicSceneAdd(elevator, elevatorRender, &elevator->rigidBody.transform.position, 3.9f);
    elevator->flags = elevatorDefinition->targetElevator == -1 ? ElevatorFlagsIsArrival : 0;
    elevator->targetElevator = elevatorDefinition->targetElevator;
    elevator->openAmount = 0.0f;

    elevator->timer = elevatorDefinition->targetElevator == -1 ? OPEN_DELAY : TELEPORT_DELAY;

    dynamicSceneSetRoomFlags(elevator->dynamicId, ROOM_FLAG_FROM_INDEX(elevatorDefinition->roomIndex));
}

int elevatorUpdate(struct Elevator* elevator, struct Player* player) {
    struct Vector3 playerOffset;
    vector3Sub(&elevator->rigidBody.transform.position, &player->lookTransform.position, &playerOffset);

    float verticalDistance = fabsf(playerOffset.y);
    playerOffset.y = 0.0f;

    float horizontalDistance = vector3MagSqrd(&playerOffset);

    int inRange = horizontalDistance < AUTO_OPEN_DISTANCE * AUTO_OPEN_DISTANCE && verticalDistance < SAME_LEVEL_HEIGHT;
    int inside = horizontalDistance < INSIDE_DISTANCE * INSIDE_DISTANCE && verticalDistance < SAME_LEVEL_HEIGHT;

    int isClosed = elevator->openAmount == 0.0f;
    int isOpen = elevator->openAmount == 1.0f;
    int shouldBeOpen = 0;

    short destination = -1;

    if ((elevator->flags & ElevatorFlagsIsArrival)) {
        if (!(elevator->flags & ElevatorFlagsIsLocked)) {
            if (inRange) {
                if (inside && elevator->timer > 0.0f) {
                    elevator->timer -= FIXED_DELTA_TIME;
                }

                shouldBeOpen = elevator->timer <= 0.0f;
            } else if (elevator->flags & ElevatorFlagsHasHadPlayer) {
                elevator->flags |= ElevatorFlagsIsLocked;

                // Save the checkpoint after flag ElevatorFlagsIsLocked is set
                // so loading this checkpoint doesn't immediately create another
                sceneQueueCheckpoint(&gScene);
            }
        }
    } else {
        if (inside) {
            elevator->flags |= ElevatorFlagsIsLocked;
        }

        if (elevator->flags & ElevatorFlagsIsLocked) {
            int cutscenePreventingMovement = (elevator->targetElevator >= gScene.elevatorCount) && cutsceneIsDialogueQueued();

            if (isClosed && !cutscenePreventingMovement && elevator->timer > 0.0f) {
                elevator->timer -= FIXED_DELTA_TIME;

                if (elevator->timer <= 0.0f) {
                    destination = elevator->targetElevator;
                }
            }
        } else {
            shouldBeOpen = (elevator->flags & ElevatorFlagsPlayerWasInRange);
        }
    }

    if (inside) {
        elevator->flags |= ElevatorFlagsHasHadPlayer;
    }

    // Update doors
    if (inRange) {
        elevator->flags |= ElevatorFlagsPlayerWasInRange;

        // Since this logic is modifying a single copy of the collider it should
        // only update the collision layers for the elevator close to the player
        if (shouldBeOpen) {
            props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_RIGHT_COLLISION_INDEX].collisionLayers = 0;
            props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_LEFT_COLLISION_INDEX].collisionLayers = 0;
        } else {
            props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_RIGHT_COLLISION_INDEX].collisionLayers = sElevatorCollisionLayers;
            props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_LEFT_COLLISION_INDEX].collisionLayers = sElevatorCollisionLayers;
        }
    }
    elevator->openAmount = mathfMoveTowards(elevator->openAmount, shouldBeOpen ? 1.0f : 0.0f, DOOR_SPEED * FIXED_DELTA_TIME);

    // Sounds and effects
    if ((isClosed && shouldBeOpen) || (isOpen && !shouldBeOpen)) {
        soundPlayerPlay(soundsElevatorDoor, 1.0f, 0.5f, &elevator->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
        if ((isClosed && shouldBeOpen) && (elevator->flags & ElevatorFlagsIsArrival)){
            soundPlayerPlay(soundsElevatorChime, 1.5f, 0.5f, &elevator->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
            hudShowSubtitle(&gScene.hud, PORTAL_ELEVATOR_CHIME, SubtitleTypeCaption);
        }
    }

    if (((elevator->flags & (ElevatorFlagsIsLocked | ElevatorFlagsIsArrival | ElevatorFlagsMovingSoundPlayed)) == ElevatorFlagsIsLocked) &&
        elevator->timer <= MOVE_SOUND_START
    ) {
        soundPlayerPlay(soundsElevatorMoving, 1.25f, 0.5f, &elevator->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
        hudShowSubtitle(&gScene.hud, PORTAL_ELEVATOR_START, SubtitleTypeCaption);
        player->shakeTimer = MOVE_SHAKE_DURATION;
        rumblePakClipPlay(&sElevatorRumbleWave);
        elevator->flags |= ElevatorFlagsMovingSoundPlayed;
    }

    return destination;
}

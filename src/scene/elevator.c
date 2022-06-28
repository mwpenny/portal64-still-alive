#include "elevator.h"
#include "../physics/collision_scene.h"
#include "../scene/dynamic_scene.h"
#include "../physics/mesh_collider.h"
#include "./signals.h"
#include "../math/mathf.h"
#include "../util/time.h"

#include "../../build/assets/models/props/round_elevator_collision.h"
#include "../../build/assets/models/props/round_elevator_interior.h"
#include "../../build/assets/models/props/round_elevator.h"

#include "../../build/assets/materials/static.h"

#define AUTO_OPEN_DISTANCE      4.0f
#define INSIDE_DISTANCE      1.0f
#define SAME_LEVEL_HEIGHT    3.0f
#define OPEN_SPEED           2.0f

struct ColliderTypeData gElevatorColliderType = {
    CollisionShapeTypeMesh,
    &props_round_elevator_collision_collider,
    0.0f, 0.6f,
    &gMeshColliderCallbacks
};

int gElevatorCollisionLayers = COLLISION_LAYERS_STATIC | COLLISION_LAYERS_TANGIBLE;

struct Vector3 gClosedPosition[] = {
    [PROPS_ROUND_ELEVATOR_ELEVATOR_BONE] = {0, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORLEFT_BONE] = {1.46439 * SCENE_SCALE, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORRIGHT_BONE] = {1.46439 * SCENE_SCALE, 0, 0},
};

struct Vector3 gOpenPosition[] = {
    [PROPS_ROUND_ELEVATOR_ELEVATOR_BONE] = {0, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORLEFT_BONE] = {1.188716 * SCENE_SCALE, 0.653916 * SCENE_SCALE, 0.0f},
    [PROPS_ROUND_ELEVATOR_DOORRIGHT_BONE] = {1.188716 * SCENE_SCALE, -0.653916 * SCENE_SCALE, 0.0f},
};

void elevatorRender(void* data, struct RenderScene* renderScene) {
    struct Elevator* elevator = (struct Elevator*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&elevator->rigidBody.transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderScene->renderState, PROPS_ROUND_ELEVATOR_DEFAULT_BONES_COUNT);

    for (int i = 0; i < PROPS_ROUND_ELEVATOR_DEFAULT_BONES_COUNT; ++i) {
        vector3Lerp(&gClosedPosition[i], &gOpenPosition[i], elevator->openAmount, &props_round_elevator_default_bones[i].position);
        transformToMatrixL(&props_round_elevator_default_bones[i], &armature[i], 1.0f);
    }

    renderSceneAdd(
        renderScene, 
        props_round_elevator_model_gfx, 
        matrix, 
        DEFAULT_INDEX, 
        &elevator->rigidBody.transform.position, 
        armature
    );

    int insideCheck = elevator->flags & (ElevatorFlagsIsLocked | ElevatorFlagsIsExit);

    int isPlayerInside = insideCheck == ElevatorFlagsIsLocked || insideCheck == ElevatorFlagsIsExit;

    if (elevator->openAmount > 0.0f || isPlayerInside) {
        renderSceneAdd(
            renderScene, 
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

    collisionObjectUpdateBB(&elevator->collisionObject);

    elevator->dynamicId = dynamicSceneAdd(elevator, elevatorRender, &elevator->rigidBody.transform, 3.9f);
    elevator->flags = elevatorDefinition->isExit ? ElevatorFlagsIsExit : 0;
    elevator->openAmount = 0.0f;
    elevator->roomIndex = elevatorDefinition->roomIndex;
    elevator->signalIndex = elevatorDefinition->signalIndex;
}

void elevatorUpdate(struct Elevator* elevator, struct Player* player) {
    struct Vector3 offset;
    vector3Sub(&elevator->rigidBody.transform.position, &player->lookTransform.position, &offset);
    
    float verticalDistance = fabsf(offset.y);
    offset.y = 0.0f;

    float horizontalDistance = vector3MagSqrd(&offset);

    int inRange = horizontalDistance < AUTO_OPEN_DISTANCE * AUTO_OPEN_DISTANCE && verticalDistance < SAME_LEVEL_HEIGHT;
    int inside = horizontalDistance < INSIDE_DISTANCE * INSIDE_DISTANCE && verticalDistance < SAME_LEVEL_HEIGHT;

    int shouldBeOpen;
    int shouldLock;

    if (elevator->flags & ElevatorFlagsIsExit) {
        shouldBeOpen = signalsRead(elevator->signalIndex);
        shouldLock = !inRange && (elevator->flags & ElevatorFlagsHasHadPlayer) != 0;
    } else {
        shouldBeOpen = inRange && !inside;
        shouldLock = inside;
        
        if (inside || (elevator->flags & ElevatorFlagsIsLocked) != 0) {
            signalsSend(elevator->signalIndex);
        }
    }

    if (inside) {
        elevator->flags |= ElevatorFlagsHasHadPlayer;
    }

    if (shouldLock) {
        elevator->flags |= ElevatorFlagsIsLocked;
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

    elevator->openAmount = mathfMoveTowards(elevator->openAmount, shouldBeOpen ? 1.0f : 0.0f, OPEN_SPEED * FIXED_DELTA_TIME);
}
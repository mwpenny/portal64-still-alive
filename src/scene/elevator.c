#include "elevator.h"
#include "../physics/collision_scene.h"
#include "../scene/dynamic_scene.h"
#include "../physics/mesh_collider.h"

#include "../../build/assets/models/props/round_elevator_collision.h"
#include "../../build/assets/models/props/round_elevator_interior.h"
#include "../../build/assets/models/props/round_elevator.h"

#include "../../build/assets/materials/static.h"

#define AUTO_OPEN_DISTANCE      4.0f

#define INSIDE_DISTANCE      1.0f

struct ColliderTypeData gElevatorColliderType = {
    CollisionShapeTypeMesh,
    &props_round_elevator_collision_collider,
    0.0f, 0.6f,
    &gMeshColliderCallbacks
};

int gElevatorCollisionLayers = COLLISION_LAYERS_STATIC | COLLISION_LAYERS_TANGIBLE;

struct Vector3 gClosedPosition[] = {
    [PROPS_ROUND_ELEVATOR_ELEVATOR_BONE] = {0, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORLEFT_BONE] = {1.46439, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORRIGHT_BONE] = {1.46439, 0, 0},
};

struct Vector3 gOpenPosition[] = {
    [PROPS_ROUND_ELEVATOR_ELEVATOR_BONE] = {0, 0, 0},
    [PROPS_ROUND_ELEVATOR_DOORLEFT_BONE] = {-0.275674 * SCENE_SCALE, 0.653916 * SCENE_SCALE, 0.0f},
    [PROPS_ROUND_ELEVATOR_DOORRIGHT_BONE] = {-0.275674 * SCENE_SCALE, -0.653916 * SCENE_SCALE, 0.0f},
};

void elevatorRender(void* data, struct RenderScene* renderScene) {
    struct Elevator* elevator = (struct Elevator*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&elevator->rigidBody.transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderScene->renderState, PROPS_ROUND_ELEVATOR_DEFAULT_BONES_COUNT);

    for (int i = 0; i < PROPS_ROUND_ELEVATOR_DEFAULT_BONES_COUNT; ++i) {
        if (elevator->flags & ElevatorFlagsIsOpen) {
            props_round_elevator_default_bones[i].position = gOpenPosition[i];
        } else {
            props_round_elevator_default_bones[i].position = gClosedPosition[i];
        }

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

    if (elevator->flags & (ElevatorFlagsIsOpen | ElevatorFlagsContainsPlayer)) {
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

void elevatorInit(struct Elevator* elevator) {
    collisionObjectInit(&elevator->collisionObject, &gElevatorColliderType, &elevator->rigidBody, 1.0f, gElevatorCollisionLayers);
    rigidBodyMarkKinematic(&elevator->rigidBody);
    collisionSceneAddDynamicObject(&elevator->collisionObject);

    elevator->rigidBody.transform.position.x = -10.0f;
    elevator->rigidBody.transform.position.z = 2.5f;

    collisionObjectUpdateBB(&elevator->collisionObject);

    elevator->dynamicId = dynamicSceneAdd(elevator, elevatorRender, &elevator->rigidBody.transform, 3.9f);
    elevator->flags = 0;
}

void elevatorUpdate(struct Elevator* elevator, struct Player* player) {
    struct Vector3 offset;
    vector3Sub(&elevator->rigidBody.transform.position, &player->lookTransform.position, &offset);
    offset.y = 0.0f;

    float horizontalDistance = vector3MagSqrd(&offset);

    int inRange = horizontalDistance < AUTO_OPEN_DISTANCE * AUTO_OPEN_DISTANCE;
    int inside = horizontalDistance < INSIDE_DISTANCE * INSIDE_DISTANCE;

    if (inside) {
        elevator->flags |= ElevatorFlagsContainsPlayer;
        player->body.currentRoom = RIGID_BODY_NO_ROOM;
    }

    int shouldBeOpen = inRange && (elevator->flags & ElevatorFlagsContainsPlayer) == 0;

    if (shouldBeOpen) {
        props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_RIGHT_COLLISION_INDEX].collisionLayers = 0;
        props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_LEFT_COLLISION_INDEX].collisionLayers = 0;

        elevator->flags |= ElevatorFlagsIsOpen;
    } else {
        props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_RIGHT_COLLISION_INDEX].collisionLayers = gElevatorCollisionLayers;
        props_round_elevator_collision_collider.children[PROPS_ROUND_ELEVATOR_COLLISION_DOOR_LEFT_COLLISION_INDEX].collisionLayers = gElevatorCollisionLayers;

        elevator->flags &= ~ElevatorFlagsIsOpen;
    }
}
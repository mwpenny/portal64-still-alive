#include "rigid_body.h"

#include "collision_scene.h"
#include "contact_solver.h"
#include "defs.h"
#include "physics/config.h"
#include "system/time.h"

#define VELOCITY_SLEEP_THRESHOLD      0.001f
#define ANGULAR_VELOCITY_SLEEP_THRESHOLD  0.001f

void rigidBodyInit(struct RigidBody* rigidBody, float mass, float momentOfIniteria) {
    transformInitIdentity(&rigidBody->transform);
    rigidBody->velocity = gZeroVec;
    rigidBody->angularVelocity = gZeroVec;

    rigidBody->mass = mass;
    rigidBody->massInv = 1.0f / mass;

    rigidBody->momentOfInertia = momentOfIniteria;
    rigidBody->momentOfInertiaInv = 1.0f / rigidBody->momentOfInertia;

    rigidBody->flags = 0;

    rigidBody->currentRoom = RIGID_BODY_NO_ROOM;
    rigidBody->sleepFrames = IDLE_SLEEP_FRAMES;

    basisFromQuat(&rigidBody->rotationBasis, &rigidBody->transform.rotation);
}

void rigidBodyMarkKinematic(struct RigidBody* rigidBody) {
    rigidBody->flags |= RigidBodyIsKinematic;
    rigidBody->mass = 1000000000000000.0f;
    rigidBody->massInv = 0.0f;
    rigidBody->momentOfInertia = 1000000000000000.0f;
    rigidBody->momentOfInertiaInv = 0.0f;
}

void rigidBodyUnmarkKinematic(struct RigidBody* rigidBody, float mass, float momentOfIniteria) {
    rigidBody->flags &= ~RigidBodyIsKinematic;

    rigidBody->mass = mass;
    rigidBody->massInv = 1.0f / mass;

    rigidBody->momentOfInertia = momentOfIniteria;
    rigidBody->momentOfInertiaInv = 1.0f / rigidBody->momentOfInertia;
}

void rigidBodyAppyImpulse(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* impulse) {
    struct Vector3 offset;
    vector3Sub(worldPoint, &rigidBody->transform.position, &offset);

    struct Vector3 torque;
    vector3Cross(&offset, impulse, &torque);

    vector3AddScaled(&rigidBody->angularVelocity, &torque, rigidBody->momentOfInertiaInv, &rigidBody->angularVelocity);
    vector3AddScaled(&rigidBody->velocity, impulse, rigidBody->massInv, &rigidBody->velocity);
}

#define ENERGY_SCALE_PER_STEP   0.99f

void rigidBodyUpdate(struct RigidBody* rigidBody) {
    if (!(rigidBody->flags & RigidBodyDisableGravity)) {
        rigidBody->velocity.y += GRAVITY_CONSTANT * FIXED_DELTA_TIME;
    }

    // first check if body is ready to sleep
    if (fabsf(rigidBody->velocity.x) < VELOCITY_SLEEP_THRESHOLD &&  fabsf(rigidBody->velocity.y) < VELOCITY_SLEEP_THRESHOLD &&  fabsf(rigidBody->velocity.z) < VELOCITY_SLEEP_THRESHOLD && 
        fabsf(rigidBody->angularVelocity.x) < ANGULAR_VELOCITY_SLEEP_THRESHOLD &&  fabsf(rigidBody->angularVelocity.y) < ANGULAR_VELOCITY_SLEEP_THRESHOLD &&  fabsf(rigidBody->angularVelocity.z) < ANGULAR_VELOCITY_SLEEP_THRESHOLD) {
        --rigidBody->sleepFrames;

        if (rigidBody->sleepFrames == 0) {
            rigidBody->flags |= RigidBodyIsSleeping;
            return;
        }
    } else {
        rigidBody->sleepFrames = IDLE_SLEEP_FRAMES;
    }

    vector3AddScaled(&rigidBody->transform.position, &rigidBody->velocity, FIXED_DELTA_TIME, &rigidBody->transform.position);
    quatApplyAngularVelocity(&rigidBody->transform.rotation, &rigidBody->angularVelocity, FIXED_DELTA_TIME, &rigidBody->transform.rotation);

    // vector3Scale(&rigidBody->velocity, &rigidBody->velocity, ENERGY_SCALE_PER_STEP);
    vector3Scale(&rigidBody->angularVelocity, &rigidBody->angularVelocity, ENERGY_SCALE_PER_STEP);

    if (rigidBody->transform.position.y < KILL_PLANE_Y) {
        rigidBody->transform.position.y = KILL_PLANE_Y;
        rigidBody->velocity.y = 0.0f;

        rigidBody->flags |= RigidBodyFizzled;
    }
}

void rigidBodyVelocityAtLocalPoint(struct RigidBody* rigidBody, struct Vector3* localPoint, struct Vector3* worldVelocity) {
    vector3Cross(&rigidBody->angularVelocity, localPoint, worldVelocity);
    vector3Add(worldVelocity, &rigidBody->velocity, worldVelocity);
}

void rigidBodyVelocityAtWorldPoint(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* worldVelocity) {
    struct Vector3 relativePos;
    vector3Sub(worldPoint, &rigidBody->transform.position, &relativePos);
    vector3Cross(&rigidBody->angularVelocity, &relativePos, worldVelocity);
    vector3Add(worldVelocity, &rigidBody->velocity, worldVelocity);
}

float rigidBodyMassInverseAtLocalPoint(struct RigidBody* rigidBody, struct Vector3* localPoint, struct Vector3* normal) {
    struct Vector3 crossPoint;
    vector3Cross(localPoint, normal, &crossPoint);
    return rigidBody->massInv + rigidBody->momentOfInertiaInv * vector3MagSqrd(&crossPoint);
}

void rigidBodyClampToPortal(struct RigidBody* rigidBody, struct Transform* portal, struct Vector3* localPoint) {
    //clamping the x and y of local point to a slightly smaller oval on the output portal
    struct Vector3 clampedLocalPoint;
    clampedLocalPoint.x = localPoint->x;
    clampedLocalPoint.y = localPoint->y * 0.5f;
    clampedLocalPoint.z = 0.0f;
    while(vector3MagSqrd(&clampedLocalPoint)>PORTAL_EXIT_XY_CLAMP_DISTANCE*PORTAL_EXIT_XY_CLAMP_DISTANCE){
        vector3Scale(&clampedLocalPoint, &clampedLocalPoint, 0.90f);
    }
    localPoint->x = clampedLocalPoint.x;
    localPoint->y = clampedLocalPoint.y * 2.0f;
    transformPoint(portal, localPoint, &rigidBody->transform.position);
}

int rigidBodyCheckPortals(struct RigidBody* rigidBody) {
    if (!gCollisionScene.portalTransforms[0] || !gCollisionScene.portalTransforms[1]) {
        rigidBody->flags &= ~(RigidBodyFlagsInFrontPortal0 | RigidBodyFlagsInFrontPortal1);
        rigidBody->flags |= RigidBodyFlagsPortalsInactive;
        return 0;
    }

    enum RigidBodyFlags newFlags = 0;

    if (rigidBody->flags & RigidBodyIsTouchingPortalA) {
        newFlags |= RigidBodyWasTouchingPortalA;
    }

    if (rigidBody->flags & RigidBodyIsTouchingPortalB) {
        newFlags |= RigidBodyWasTouchingPortalB;
    }

    int result = 0;

    for (int i = 0; i < 2; ++i) {
        struct Quaternion inverseRotation;
        quatConjugate(&gCollisionScene.portalTransforms[i]->rotation, &inverseRotation);
        struct Vector3 localPoint;
        vector3Sub(&rigidBody->transform.position, &gCollisionScene.portalTransforms[i]->position, &localPoint);
        quatMultVector(&inverseRotation, &localPoint, &localPoint);

        int mask = (RigidBodyFlagsInFrontPortal0 << i);

        if (localPoint.z < 0.0f) {
            newFlags |= mask;
        }

        if (!((RigidBodyIsTouchingPortalA << i) & rigidBody->flags) && !((RigidBodyWasTouchingPortalA << i) & rigidBody->flags)) {
            continue;
        }

        // skip checking if portal was crossed if this is the
        // first frame portals were active or the object was
        // just teleported
        if (rigidBody->flags & (
            RigidBodyFlagsPortalsInactive | 
            (RigidBodyFlagsCrossedPortal0 << (1 - i))) ||
            (newFlags & RigidBodyFlagsCrossedPortal0)
        ) {
            continue;
        }

        struct Vector3 localVelocity;
        quatMultVector(&inverseRotation, &rigidBody->velocity, &localVelocity);

        // if object is moving towards the portal make sure it is within the bounds of it
        if (vector3Dot(&localVelocity, &localPoint) < 0.0f && !((RigidBodyIsTouchingPortalA << (1 - i)) & rigidBody->flags)) {
            rigidBodyClampToPortal(rigidBody, gCollisionScene.portalTransforms[i], &localPoint);
        }

        // 0 !newFlags & flags
        // 1 newFlags & !flags

        // the xorMask changes which direction
        // each portal needs to be crossed in 
        // order to transmit an object
        int xorMask = i == 0 ? 0 : mask;

        // check if the body crossed the portal
        if (!((~newFlags ^ xorMask) & (rigidBody->flags ^ xorMask) & mask)) {
            continue;
        }

        struct Transform* otherPortal = gCollisionScene.portalTransforms[1 - i];
        rigidBodyTeleport(rigidBody, gCollisionScene.portalTransforms[i], otherPortal, &gCollisionScene.portalVelocity[i], &gCollisionScene.portalVelocity[1 - i], gCollisionScene.portalRooms[1 - i]);

        float speedSqrd = vector3MagSqrd(&rigidBody->velocity);

        if (speedSqrd > MAX_PORTAL_SPEED * MAX_PORTAL_SPEED) {
            vector3Normalize(&rigidBody->velocity, &rigidBody->velocity);
            vector3Scale(&rigidBody->velocity, &rigidBody->velocity, MAX_PORTAL_SPEED);
        }

        if (speedSqrd < MIN_PORTAL_SPEED * MIN_PORTAL_SPEED) {
            struct Vector3 portalNormal = gZeroVec;
            portalNormal.z = i ? -1.0f : 1.0f;
            quatMultVector(&otherPortal->rotation, &portalNormal, &portalNormal);

            if (portalNormal.y > 0.9f) {
                if (speedSqrd < 0.000001f) {
                    vector3Scale(&portalNormal, &rigidBody->velocity, MIN_PORTAL_SPEED);
                } else {
                    vector3Normalize(&rigidBody->velocity, &rigidBody->velocity);
                    vector3Scale(&rigidBody->velocity, &rigidBody->velocity, MIN_PORTAL_SPEED);
                }
            }
        }

        newFlags |= RigidBodyFlagsCrossedPortal0 << i;
        newFlags |= RigidBodyIsTouchingPortalA << (1 - i);
        newFlags &= ~(RigidBodyWasTouchingPortalA << i);
        result = i + 1;
    }

    rigidBody->flags &= ~(
        RigidBodyFlagsInFrontPortal0 | 
        RigidBodyFlagsInFrontPortal1 | 
        RigidBodyFlagsPortalsInactive |
        RigidBodyFlagsCrossedPortal0 |
        RigidBodyFlagsCrossedPortal1 |
        RigidBodyIsTouchingPortalA |
        RigidBodyIsTouchingPortalB |
        RigidBodyWasTouchingPortalA |
        RigidBodyWasTouchingPortalB
    );
    rigidBody->flags |= newFlags;

    return result;
}

void rigidBodyTeleport(struct RigidBody* rigidBody, struct Transform* from, struct Transform* to, struct Vector3* fromVelocity, struct Vector3* toVelocity, int toRoom) {
    struct Vector3 localPoint;

    transformPointInverseNoScale(from, &rigidBody->transform.position, &localPoint);

    transformPoint(to, &localPoint, &rigidBody->transform.position);

    struct Quaternion inverseARotation;
    quatConjugate(&from->rotation, &inverseARotation);

    struct Quaternion rotationTransfer;
    quatMultiply(&to->rotation, &inverseARotation, &rotationTransfer);

    struct Vector3 relativeVelocity;
    vector3Sub(&rigidBody->velocity, fromVelocity, &relativeVelocity);

    quatMultVector(&rotationTransfer, &relativeVelocity, &relativeVelocity);

    vector3Add(&relativeVelocity, toVelocity, &rigidBody->velocity);

    quatMultVector(&rotationTransfer, &rigidBody->angularVelocity, &rigidBody->angularVelocity);

    struct Quaternion newRotation;

    quatMultiply(&rotationTransfer, &rigidBody->transform.rotation, &newRotation);

    rigidBody->transform.rotation = newRotation;

    rigidBody->currentRoom = toRoom;
}
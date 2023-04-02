#include "rigid_body.h"
#include "../util/time.h"
#include "../physics/config.h"
#include "contact_solver.h"
#include "collision_scene.h"
#include "defs.h"
#include <math.h>

void rigidBodyInit(struct RigidBody* rigidBody, float mass, float momentOfIniteria) {
    transformInitIdentity(&rigidBody->transform);
    rigidBody->velocity = gZeroVec;
    rigidBody->angularVelocity = gZeroVec;

    rigidBody->mass = mass;
    rigidBody->massInv = 1.0f / mass;

    rigidBody->momentOfInertia = momentOfIniteria;
    rigidBody->momentOfInertiaInv = 1.0f / rigidBody->momentOfInertia;

    rigidBody->flags = 0;

    rigidBody->currentRoom = 0;

    basisFromQuat(&rigidBody->rotationBasis, &rigidBody->transform.rotation);
}

void rigidBodyMarkKinematic(struct RigidBody* rigidBody) {
    rigidBody->flags |= RigidBodyIsKinematic;
    rigidBody->mass = 1000000000000000.0f;
    rigidBody->massInv = 0.0f;
    rigidBody->momentOfInertia = 1000000000000000.0f;
    rigidBody->momentOfInertiaInv = 0.0f;
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

void rigidBodyClampToPortal(struct RigidBody* rigidBody, struct Transform* portal) {
    struct Vector3 localPoint;

    transformPointInverseNoScale(portal, &rigidBody->transform.position, &localPoint);

    //clamping the x and y of local point to a slightly smaller oval on the output portal
    struct Vector3 clampedLocalPoint;
    clampedLocalPoint = localPoint;
    clampedLocalPoint.y /= 2.0f;
    clampedLocalPoint.z = 0.0f;
    while(sqrtf(vector3MagSqrd(&clampedLocalPoint))>PORTAL_EXIT_XY_CLAMP_DISTANCE){
        vector3Scale(&clampedLocalPoint, &clampedLocalPoint, 0.90f);
    }
    clampedLocalPoint.y *= 2.0f;
    localPoint.x = clampedLocalPoint.x;
    localPoint.y = clampedLocalPoint.y;
    transformPoint(portal, &localPoint, &rigidBody->transform.position);
}

int rigidBodyCheckPortals(struct RigidBody* rigidBody) {
    if (!gCollisionScene.portalTransforms[0] || !gCollisionScene.portalTransforms[1]) {
        rigidBody->flags &= ~(RigidBodyFlagsInFrontPortal0 | RigidBodyFlagsInFrontPortal1);
        rigidBody->flags |= RigidBodyFlagsPortalsInactive;
        return 0;
    }

    struct Vector3 localPoint;

    enum RigidBodyFlags newFlags = 0;

    //if only touching one portal, clamp object to edges of that portal
    if ((rigidBody->flags & RigidBodyIsTouchingPortalA) && !(rigidBody->flags & RigidBodyIsTouchingPortalB)){
        rigidBodyClampToPortal(rigidBody, gCollisionScene.portalTransforms[0]);
    }
    else if ((rigidBody->flags & RigidBodyIsTouchingPortalB) && !(rigidBody->flags & RigidBodyIsTouchingPortalA)){
        rigidBodyClampToPortal(rigidBody, gCollisionScene.portalTransforms[1]);
    }

    if (rigidBody->flags & RigidBodyIsTouchingPortalA) {
        newFlags |= RigidBodyWasTouchingPortalA;
    }

    if (rigidBody->flags & RigidBodyIsTouchingPortalB) {
        newFlags |= RigidBodyWasTouchingPortalB;
    }

    int result = 0;

    for (int i = 0; i < 2; ++i) {
        transformPointInverseNoScale(gCollisionScene.portalTransforms[i], &rigidBody->transform.position, &localPoint);

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

        newFlags |= RigidBodyFlagsCrossedPortal0 << i;
        newFlags |= RigidBodyIsTouchingPortalA << (1 - i);
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
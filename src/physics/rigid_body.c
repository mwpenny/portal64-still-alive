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
    rigidBody->velocity.y += GRAVITY_CONSTANT * FIXED_DELTA_TIME;

    vector3AddScaled(&rigidBody->transform.position, &rigidBody->velocity, FIXED_DELTA_TIME, &rigidBody->transform.position);
    quatApplyAngularVelocity(&rigidBody->transform.rotation, &rigidBody->angularVelocity, FIXED_DELTA_TIME, &rigidBody->transform.rotation);

    vector3Scale(&rigidBody->velocity, &rigidBody->velocity, ENERGY_SCALE_PER_STEP);
    vector3Scale(&rigidBody->angularVelocity, &rigidBody->angularVelocity, ENERGY_SCALE_PER_STEP);
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


void rigidBodyCheckPortals(struct RigidBody* rigidBody) {
    if (!gCollisionScene.portalTransforms[0] || !gCollisionScene.portalTransforms[1]) {
        rigidBody->flags &= ~(RigidBodyFlagsInFrontPortal0 | RigidBodyFlagsInFrontPortal1);
        rigidBody->flags |= RigidBodyFlagsPortalsInactive;
        return;
    }

    struct Vector3 localPoint;

    enum RigidBodyFlags newFlags = 0;

    for (int i = 0; i < 2; ++i) {
        transformPointInverse(gCollisionScene.portalTransforms[i], &rigidBody->transform.position, &localPoint);

        int mask = (1 << i);

        if (localPoint.z < 0.0f) {
            newFlags |= mask;
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

        struct Vector3 scaledPoint;

        scaledPoint.x = localPoint.x * (1.0f / PORTAL_X_RADIUS);
        scaledPoint.y = localPoint.y;
        scaledPoint.z = 0.0f;

        if (vector3MagSqrd(&scaledPoint) > 1.0f) {
            continue;
        }

        struct Transform* otherPortal = gCollisionScene.portalTransforms[1 - i];

        transformPoint(otherPortal, &localPoint, &rigidBody->transform.position);

        struct Quaternion inverseARotation;
        quatConjugate(&gCollisionScene.portalTransforms[i]->rotation, &inverseARotation);

        struct Quaternion rotationTransfer;
        quatMultiply(&inverseARotation, &otherPortal->rotation, &rotationTransfer);

        quatMultVector(&rotationTransfer, &rigidBody->velocity, &rigidBody->velocity);
        quatMultVector(&rotationTransfer, &rigidBody->angularVelocity, &rigidBody->angularVelocity);

        struct Quaternion newRotation;

        quatMultiply(&rotationTransfer, &rigidBody->transform.rotation, &newRotation);

        rigidBody->transform.rotation = newRotation;

        newFlags |= RigidBodyFlagsCrossedPortal0 << i;
    }

    rigidBody->flags &= ~(
        RigidBodyFlagsInFrontPortal0 | 
        RigidBodyFlagsInFrontPortal1 | 
        RigidBodyFlagsPortalsInactive |
        RigidBodyFlagsCrossedPortal0 |
        RigidBodyFlagsCrossedPortal1
    );
    rigidBody->flags |= newFlags;
}
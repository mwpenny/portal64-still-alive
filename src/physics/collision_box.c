#include "collision_box.h"

#include "raycasting.h"
#include "../math/mathf.h"

#include <math.h>

struct ColliderCallbacks gCollisionBoxCallbacks = {
    collisionBoxCollidePlane,
    collisionBoxCollideQuad,
    raycastBox,
    collisionBoxSolidMofI,
    collisionBoxBoundingBox,
    collisionBoxMinkowsiSum,
};

int _collsionBuildPlaneContact(struct Transform* boxTransform, struct Plane* plane, struct Vector3* point, struct ContactConstraintState* output, int id) {
    struct Vector3 worldPoint;
    struct ContactState* contact = &output->contacts[output->contactCount];

    quatMultVector(&boxTransform->rotation, point, &contact->rb);
    vector3Add(&contact->rb, &boxTransform->position, &worldPoint);
    float penetration = planePointDistance(plane, &worldPoint);

    if (penetration >= NEGATIVE_PENETRATION_BIAS) {
        return 0;
    }

    vector3AddScaled(&worldPoint, &plane->normal, -penetration, &contact->ra);

    ++output->contactCount;
    contact->id = id;
    contact->penetration = penetration;
    contact->bias = 0;
    contact->normalMass = 0;
    contact->tangentMass[0] = 0.0f;
    contact->tangentMass[1] = 0.0f;
    contact->normalImpulse = 0.0f;
    contact->tangentImpulse[0] = 0.0f;
    contact->tangentImpulse[1] = 0.0f;

    return 1;
}


int collisionBoxCollidePlane(void* data, struct Transform* boxTransform, struct Plane* plane, struct ContactConstraintState* output) {
    struct CollisionBox* box = (struct CollisionBox*)data;

    float boxDistance = planePointDistance(plane, &boxTransform->position);
    float maxBoxReach = vector3MagSqrd(&box->sideLength);

    if (boxDistance > maxBoxReach || boxDistance < 0.0f) {
        return 0;
    }

    struct Quaternion inverseBoxRotation;

    quatConjugate(&boxTransform->rotation, &inverseBoxRotation);

    struct Vector3 normalInBoxSpace;

    quatMultVector(&inverseBoxRotation, &plane->normal, &normalInBoxSpace);

    struct Vector3 deepestCorner;

    int id = 0;

    for (int axis = 0; axis < 3; ++axis) {
        float normalValue = VECTOR3_AS_ARRAY(&normalInBoxSpace)[axis];
        if (normalValue < 0) {
            VECTOR3_AS_ARRAY(&deepestCorner)[axis] = VECTOR3_AS_ARRAY(&box->sideLength)[axis];
        } else {
            VECTOR3_AS_ARRAY(&deepestCorner)[axis] = -VECTOR3_AS_ARRAY(&box->sideLength)[axis];
            id |= 1 << axis;
        }
    }

    output->contactCount = 0;
    output->normal = plane->normal;
    // TODO actually calculate tangent 
    output->tangentVectors[0] = gRight;
    output->tangentVectors[1] = gForward;

    output->restitution = 0.1f;
    output->friction = 0.5f;

    if (!_collsionBuildPlaneContact(boxTransform, plane, &deepestCorner, output, id)) {
        return 0;
    }

    struct Vector3 sideLengthProjected;
    vector3Multiply(&box->sideLength, &normalInBoxSpace, &sideLengthProjected);

    int minAxis = 0;
    float minAxisDistnace = fabsf(sideLengthProjected.x);
    int maxAxis = 0;
    float maxAxisDistnace = minAxisDistnace;

    for (int axis = 1; axis < 3; ++axis) {
        float length = fabsf(VECTOR3_AS_ARRAY(&sideLengthProjected)[axis]);

        if (length < minAxisDistnace) {
            minAxisDistnace = length;
            minAxis = axis;
        }

        if (length > maxAxisDistnace) {
            maxAxisDistnace = length;
            maxAxis = axis;
        }
    }

    int midAxis = 3 - maxAxis - minAxis;

    struct Vector3 nextFurthestPoint = deepestCorner;
    int nextId = id ^ (1 << minAxis);
    VECTOR3_AS_ARRAY(&nextFurthestPoint)[minAxis] = -VECTOR3_AS_ARRAY(&nextFurthestPoint)[minAxis];

    if (!_collsionBuildPlaneContact(boxTransform, plane, &nextFurthestPoint, output, nextId)) {
        return 1;
    }

    nextId = nextId ^ (1 << midAxis);
    VECTOR3_AS_ARRAY(&nextFurthestPoint)[midAxis] = -VECTOR3_AS_ARRAY(&nextFurthestPoint)[midAxis];
    _collsionBuildPlaneContact(boxTransform, plane, &nextFurthestPoint, output, nextId);

    nextId = nextId ^ (1 << minAxis);
    VECTOR3_AS_ARRAY(&nextFurthestPoint)[minAxis] = -VECTOR3_AS_ARRAY(&nextFurthestPoint)[minAxis];
    _collsionBuildPlaneContact(boxTransform, plane, &nextFurthestPoint, output, nextId);

    return 1;
}

float collisionBoxSolidMofI(struct ColliderTypeData* typeData, float mass) {
    struct CollisionBox* collisionBox = (struct CollisionBox*)typeData->data;

    float xx = collisionBox->sideLength.x * collisionBox->sideLength.x;
    float yy = collisionBox->sideLength.y * collisionBox->sideLength.y;
    float zz = collisionBox->sideLength.z * collisionBox->sideLength.z;

    float biggestSide = xx + yy;

    float sideCheck = yy + zz;

    if (sideCheck > biggestSide) {
        biggestSide = sideCheck;
    }

    sideCheck = zz + xx;

    if (sideCheck > biggestSide) {
        biggestSide = sideCheck;
    }


    return mass * biggestSide * (1.0f / 12.0f);
}

void collisionBoxBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box) {
    struct CollisionBox* collisionBox = (struct CollisionBox*)typeData->data;
    struct Vector3 halfSize;
    quatRotatedBoundingBoxSize(&transform->rotation, &collisionBox->sideLength, &halfSize);
    vector3Sub(&transform->position, &halfSize, &box->min);
    vector3Add(&transform->position, &halfSize, &box->max);
}

void collisionBoxMinkowsiSum(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output) {
    struct CollisionBox* collisionBox = (struct CollisionBox*)data;
    vector3Scale(&basis->x, output, vector3Dot(&basis->x, direction) > 0.0f ? collisionBox->sideLength.x : -collisionBox->sideLength.x);
    vector3AddScaled(output, &basis->y, vector3Dot(&basis->y, direction) > 0.0f ? collisionBox->sideLength.y : -collisionBox->sideLength.y, output);
    vector3AddScaled(output, &basis->z, vector3Dot(&basis->z, direction) > 0.0f ? collisionBox->sideLength.z : -collisionBox->sideLength.z, output);
}
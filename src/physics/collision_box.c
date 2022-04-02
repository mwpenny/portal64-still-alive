#include "collision_box.h"

#include <math.h>

#define NORMAL_ZERO_BIAS    0.001f

#define NEGATIVE_PENETRATION_BIAS   0.1f

struct ColliderCallbacks gCollisionBoxCallbacks = {
    collisionBoxCollidePlane,
    collisionBoxSolidMofI,
};

void _collsionBuildPlaneContact(struct Transform* boxTransform, struct Plane* plane, struct Vector3* point, struct ContactConstraintState* output, int id) {
    struct Vector3 worldPoint;
    struct ContactState* contact = &output->contacts[output->contactCount];

    quatMultVector(&boxTransform->rotation, point, &contact->rb);
    vector3Add(&contact->rb, &boxTransform->position, &worldPoint);
    float penetration = planePointDistance(plane, &worldPoint);

    if (penetration >= NEGATIVE_PENETRATION_BIAS) {
        return;
    }


    vector3AddScaled(&worldPoint, &plane->normal, penetration, &contact->ra);

    ++output->contactCount;
    contact->id = id;
    contact->penetration = penetration;
    contact->bias = 0;
    contact->normalMass = 0;
    contact->tangentMass[0] = 0.0f;
    contact->tangentMass[1] = 0.0f;
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

    output->restitution = 0.0f;
    output->friction = 1.0f;

    _collsionBuildPlaneContact(boxTransform, plane, &deepestCorner, output, id);

    return 1;
}

void collisionBoxCollideQuad(struct CollisionBox* box, struct Transform* boxTransform, struct CollisionQuad* quad) {
    struct Quaternion inverseBoxRotation;

    quatConjugate(&boxTransform->rotation, &inverseBoxRotation);

    struct Vector3 normalInBoxSpace;

    quatMultVector(&inverseBoxRotation, &quad->normal, &normalInBoxSpace);
}

float collisionBoxSolidMofI(struct ColliderTypeData* typeData, float mass) {
    float singleSide = sqrtf(vector3MagSqrd(&((struct CollisionBox*)typeData->data)->sideLength));
    return mass * singleSide * singleSide * (1.0f / 6.0f);
}
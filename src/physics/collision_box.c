#include "collision_box.h"

#include <math.h>

#define NORMAL_ZERO_BIAS    0.001f

struct ColliderCallbacks gCollisionBoxCallbacks = {
    collisionBoxCollidePlane,
    collisionBoxSolidMofI,
};

int _collsionBuildPlaneContact(struct Transform* boxTransform, struct Plane* plane, struct Vector3* point, struct ContactState* contact) {
    quatMultVector(&boxTransform->rotation, point, &contact->rb);

    struct Vector3 worldPoint;
    vector3Add(&contact->rb, &boxTransform->position, &worldPoint);

    contact->ra = gZeroVec;

    contact->penetration = planePointDistance(plane, &worldPoint);

    if (contact->penetration >= 0.0f) {
        return 0;
    }

    contact->normalImpulse = 0.0f;
    contact->tangentImpulse[0] = 0.0f;
    contact->tangentImpulse[1] = 0.0f;
    contact->bias = 0;
    contact->normalMass = 0;
    contact->tangentMass[0] = 0.0f;
    contact->tangentMass[1] = 0.0f;

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

    int splitAxis[3];
    int splitAxisCount = 0;

    for (int axis = 0; axis < 3; ++axis) {
        float normalValue = VECTOR3_AS_ARRAY(&normalInBoxSpace)[axis];
        if (normalValue < -NORMAL_ZERO_BIAS) {
            VECTOR3_AS_ARRAY(&deepestCorner)[axis] = VECTOR3_AS_ARRAY(&box->sideLength)[axis];
        } else if (normalValue > NORMAL_ZERO_BIAS) {
            VECTOR3_AS_ARRAY(&deepestCorner)[axis] = -VECTOR3_AS_ARRAY(&box->sideLength)[axis];
        } else {
            splitAxis[splitAxisCount] = axis;
            ++splitAxisCount;
        }
    }

    output->contactCount = 0;
    output->normal = plane->normal;
    // TODO actually calculate tangent 
    output->tangentVectors[0] = gRight;
    output->tangentVectors[1] = gForward;

    output->restitution = 0.0f;
    output->friction = 1.0f;

    if (splitAxisCount == 0) {
        if (!_collsionBuildPlaneContact(boxTransform, plane, &deepestCorner, &output->contacts[output->contactCount])) {
            return output->contactCount;
        }
        ++output->contactCount;
    } else if (splitAxisCount == 1) {
        VECTOR3_AS_ARRAY(&deepestCorner)[splitAxis[0]] = VECTOR3_AS_ARRAY(&box->sideLength)[splitAxis[0]];
        if (!_collsionBuildPlaneContact(boxTransform, plane, &deepestCorner, &output->contacts[output->contactCount])) {
            return output->contactCount;
        }
        ++output->contactCount;
        VECTOR3_AS_ARRAY(&deepestCorner)[splitAxis[0]] = -VECTOR3_AS_ARRAY(&box->sideLength)[splitAxis[0]];
        if (!_collsionBuildPlaneContact(boxTransform, plane, &deepestCorner, &output->contacts[output->contactCount])) {
            return output->contactCount;
        }
        ++output->contactCount;
    } else if (splitAxisCount == 2) {
        VECTOR3_AS_ARRAY(&deepestCorner)[splitAxis[0]] = VECTOR3_AS_ARRAY(&box->sideLength)[splitAxis[0]];
        VECTOR3_AS_ARRAY(&deepestCorner)[splitAxis[1]] = VECTOR3_AS_ARRAY(&box->sideLength)[splitAxis[1]];
        if (!_collsionBuildPlaneContact(boxTransform, plane, &deepestCorner, &output->contacts[output->contactCount])) {
            return output->contactCount;
        }
        ++output->contactCount;

        VECTOR3_AS_ARRAY(&deepestCorner)[splitAxis[0]] = -VECTOR3_AS_ARRAY(&box->sideLength)[splitAxis[0]];
        if (!_collsionBuildPlaneContact(boxTransform, plane, &deepestCorner, &output->contacts[output->contactCount])) {
            return output->contactCount;
        }
        ++output->contactCount;

        VECTOR3_AS_ARRAY(&deepestCorner)[splitAxis[0]] = VECTOR3_AS_ARRAY(&box->sideLength)[splitAxis[0]];
        VECTOR3_AS_ARRAY(&deepestCorner)[splitAxis[1]] = -VECTOR3_AS_ARRAY(&box->sideLength)[splitAxis[1]];
        if (!_collsionBuildPlaneContact(boxTransform, plane, &deepestCorner, &output->contacts[output->contactCount])) {
            return output->contactCount;
        }
        ++output->contactCount;

        VECTOR3_AS_ARRAY(&deepestCorner)[splitAxis[0]] = -VECTOR3_AS_ARRAY(&box->sideLength)[splitAxis[0]];
        if (!_collsionBuildPlaneContact(boxTransform, plane, &deepestCorner, &output->contacts[output->contactCount])) {
            return output->contactCount;
        }
        ++output->contactCount;
    } else {
        // should not happen
        return output->contactCount;
    }

    return output->contactCount;
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
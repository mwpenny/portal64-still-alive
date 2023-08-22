#include "collision_sphere.h"

#include "math/plane.h"
#include "math/mathf.h"
#include "collision_quad.h"

int collisionSphereCollideQuad(void* data, struct Transform* boxTransform, struct CollisionQuad* quad, struct ContactManifold* output) {
    struct CollisionSphere* sphere = (struct CollisionSphere*)data;

    float overlap = planePointDistance(&quad->plane, &boxTransform->position) - sphere->radius;

    if (overlap > NEGATIVE_PENETRATION_BIAS || overlap < -2.0 * sphere->radius) {
        return 0;
    }

    struct Vector3 relativePos;
    vector3Sub(&boxTransform->position, &quad->corner, &relativePos);

    float aLerp = clampf(vector3Dot(&relativePos, &quad->edgeA), 0.0f, quad->edgeALength);
    float bLerp = clampf(vector3Dot(&relativePos, &quad->edgeB), 0.0f, quad->edgeBLength);

    struct ContactPoint* contact = &output->contacts[output->contactCount];

    vector3AddScaled(&quad->corner, &quad->edgeA, aLerp, &contact->contactAWorld);
    vector3AddScaled(&contact->contactAWorld, &quad->edgeB, bLerp, &contact->contactAWorld);

    vector3Sub(&boxTransform->position, &contact->contactAWorld, &output->normal);

    float outputLength = vector3MagSqrd(&output->normal);

    float extraRadius = sphere->radius + NEGATIVE_PENETRATION_BIAS;

    if (outputLength * outputLength > extraRadius * extraRadius || vector3Dot(&output->normal, &quad->plane.normal) <= 0.0f) {
        return 0;
    }

    vector3Scale(&output->normal, &output->normal, 1.0f / sqrtf(outputLength));

    vector3AddScaled(&boxTransform->position, &output->normal, -sphere->radius, &contact->contactBWorld);

    output->restitution = 0.1f;
    output->friction = 0.5f;
    ++output->contactCount;
    
    output->tangentVectors[0] = quad->edgeA;
    output->tangentVectors[1] = quad->edgeB;

    contact->id = 0;
    contact->penetration = overlap;
    contact->bias = 0;
    contact->normalMass = 0;
    contact->tangentMass[0] = 0.0f;
    contact->tangentMass[1] = 0.0f;
    contact->normalImpulse = 0.0f;
    contact->tangentImpulse[0] = 0.0f;
    contact->tangentImpulse[1] = 0.0f;

    contact->contactALocal = contact->contactAWorld;
    struct Quaternion inverseRotation;
    quatConjugate(&boxTransform->rotation, &inverseRotation);
    quatMultVector(&inverseRotation, &contact->contactBWorld, &contact->contactBLocal);

    return 1;
}

float collisionSphereSolidMofI(struct ColliderTypeData* typeData, float mass) {
    struct CollisionSphere* sphere = (struct CollisionSphere*)typeData->data;
    return (2.0f / 5.0f) * mass * sphere->radius * sphere->radius;
}

void collisionSphereBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box) {
    struct CollisionSphere* sphere = (struct CollisionSphere*)typeData->data;
    box->min.x = transform->position.x - sphere->radius;
    box->min.y = transform->position.y - sphere->radius;
    box->min.z = transform->position.z - sphere->radius;

    box->max.x = transform->position.x + sphere->radius;
    box->max.y = transform->position.y + sphere->radius;
    box->max.z = transform->position.z + sphere->radius;
}

#define SQRT_3  0.577350269f

int collisionSphereMinkowsiSum(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output) {
    struct CollisionSphere* sphere = (struct CollisionSphere*)data;

    float distance = fabsf(direction->x);
    output->x = direction->x > 0.0f ? sphere->radius : -sphere->radius;
    output->y = 0.0f;
    output->z = 0.0f;

    int result = direction->x > 0.0f ? 0x1 : 0x4;

    for (int i = 1; i < 3; ++i) {
        float distanceCheck = fabsf(VECTOR3_AS_ARRAY(direction)[i]);

        if (distanceCheck > distance) {
            distance = distanceCheck;
            *output = gZeroVec;
            VECTOR3_AS_ARRAY(output)[i] = VECTOR3_AS_ARRAY(direction)[i] > 0.0f ? sphere->radius : -sphere->radius;
        }
    }

    float distanceCheck = fabsf(direction->x + direction->y + direction->z) * SQRT_3;

    if (distanceCheck > distance) {
        float scaledRadius = sphere->radius * SQRT_3;

        output->x = direction->x > 0.0f ? scaledRadius : -scaledRadius;
        output->y = direction->y > 0.0f ? scaledRadius : -scaledRadius;
        output->z = direction->z > 0.0f ? scaledRadius : -scaledRadius;
    }
    
    return result;
}

struct ColliderCallbacks gCollisionSphereCallbacks = {
    NULL, // TODO
    collisionSphereSolidMofI,
    collisionSphereBoundingBox,
    collisionSphereMinkowsiSum,
};

int collisionSphereCheckWithNearestPoint(struct Vector3* nearestPoint, struct CollisionSphere* otherSphere, struct Vector3* spherePos, struct ContactManifold* contact) {
    vector3Sub(spherePos, nearestPoint, &contact->normal);

    float distanceSqrd = vector3MagSqrd(&contact->normal);

    if (distanceSqrd > otherSphere->radius * otherSphere->radius) {
        return 0;
    }

    float distance = 0.0f;

    if (distanceSqrd < 0.00001f) {
        contact->normal = gRight;
    } else {
        distance = sqrtf(distanceSqrd);
        vector3Scale(&contact->normal, &contact->normal, 1.0f / distance);
    }

    struct ContactPoint* contactPoint = &contact->contacts[0];

    vector3Scale(&contact->normal, &contactPoint->contactAWorld, otherSphere->radius);
    vector3Scale(&contact->normal, &contactPoint->contactBWorld, -otherSphere->radius);

    contactPoint->bias = 0.0f;
    contactPoint->id = 0;
    contactPoint->normalImpulse = 0.0f;
    contactPoint->normalMass = 0.0f;
    contactPoint->penetration = distance - otherSphere->radius;
    contactPoint->tangentImpulse[0] = 0.0f;
    contactPoint->tangentImpulse[1] = 0.0f;
    contactPoint->tangentMass[0] = 0.0f;
    contactPoint->tangentMass[0] = 0.0f;

    return 1;
}
#include "collision_sphere.h"

#include "math/plane.h"
#include "math/mathf.h"
#include "collision_quad.h"

int collisionSphereCollideQuad(void* data, struct Transform* boxTransform, struct CollisionQuad* quad, struct ContactConstraintState* output) {
    struct CollisionSphere* sphere = (struct CollisionSphere*)data;

    float overlap = planePointDistance(&quad->plane, &boxTransform->position) - sphere->radius;

    if (overlap > NEGATIVE_PENETRATION_BIAS || overlap < -2.0 * sphere->radius) {
        return 0;
    }

    struct Vector3 relativePos;
    vector3Sub(&boxTransform->position, &quad->corner, &relativePos);

    float aLerp = clampf(vector3Dot(&relativePos, &quad->edgeA), 0.0f, quad->edgeALength);
    float bLerp = clampf(vector3Dot(&relativePos, &quad->edgeB), 0.0f, quad->edgeBLength);

    struct ContactState* contact = &output->contacts[output->contactCount];

    vector3AddScaled(&quad->corner, &quad->edgeA, aLerp, &contact->ra);
    vector3AddScaled(&contact->ra, &quad->edgeB, bLerp, &contact->ra);

    vector3Sub(&boxTransform->position, &contact->ra, &output->normal);

    float outputLength = vector3MagSqrd(&output->normal);

    float extraRadius = sphere->radius + NEGATIVE_PENETRATION_BIAS;

    if (outputLength * outputLength > extraRadius * extraRadius || vector3Dot(&output->normal, &quad->plane.normal) <= 0.0f) {
        return 0;
    }

    vector3Scale(&output->normal, &output->normal, 1.0f / sqrtf(outputLength));

    vector3AddScaled(&boxTransform->position, &output->normal, -sphere->radius, &contact->rb);

    output->restitution = 0.1f;
    output->friction = 0.5f;
    ++output->contactCount;
    // TODO
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

    return 1;
}

int collisionSphereCollidePlane(void* data, struct Transform* boxTransform, struct Plane* plane, struct ContactConstraintState* output) {
    struct CollisionSphere* sphere = (struct CollisionSphere*)data;

    float overlap = planePointDistance(plane, &boxTransform->position) - sphere->radius;

    if (overlap > NEGATIVE_PENETRATION_BIAS || overlap < -2.0 * sphere->radius) {
        return 0;
    }

    struct ContactState* contact = &output->contacts[output->contactCount];

    planeProjectPoint(plane, &boxTransform->position, &contact->ra);
    output->normal = plane->normal;
    vector3AddScaled(&boxTransform->position, &output->normal, -sphere->radius, &contact->rb);

    output->restitution = 0.1f;
    output->friction = 0.5f;
    ++output->contactCount;
    // TODO
    output->tangentVectors[0] = gRight;
    output->tangentVectors[1] = gForward;

    contact->id = 0;
    contact->penetration = overlap;
    contact->bias = 0;
    contact->normalMass = 0;
    contact->tangentMass[0] = 0.0f;
    contact->tangentMass[1] = 0.0f;
    contact->normalImpulse = 0.0f;
    contact->tangentImpulse[0] = 0.0f;
    contact->tangentImpulse[1] = 0.0f;

    return 1;
}

float collisionSphereSolidMofI(struct ColliderTypeData* typeData, float mass) {
    struct CollisionSphere* sphere = (struct CollisionSphere*)typeData->data;
    return (2.0f / 5.0f) * mass * sphere->radius * sphere->radius;
}

struct ColliderCallbacks gCollisionSphereCallbacks = {
    collisionSphereCollidePlane,
    collisionSphereCollideQuad,
    NULL, // TODO
    collisionSphereSolidMofI,
};
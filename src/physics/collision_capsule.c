#include "collision_capsule.h"

#include "math/plane.h"
#include "math/mathf.h"
#include "collision_quad.h"

float collisionCapsuleSolidMofI(struct ColliderTypeData* typeData, float mass) {
    struct CollisionCapsule* capsule = (struct CollisionCapsule*)typeData->data;
    return (2.0f / 5.0f) * mass * capsule->radius * capsule->radius;
}

void collisionCapsuleBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box) {
    struct CollisionCapsule* capsule = (struct CollisionCapsule*)typeData->data;
    box->min.x = transform->position.x - capsule->radius;
    box->min.y = transform->position.y - capsule->radius - capsule->extendDownward;
    box->min.z = transform->position.z - capsule->radius;

    box->max.x = transform->position.x + capsule->radius;
    box->max.y = transform->position.y + capsule->radius;
    box->max.z = transform->position.z + capsule->radius;
}

#define SQRT_3  0.577350269f

int collisionCapsuleMinkowsiSum(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output) {
    struct CollisionCapsule* capsule = (struct CollisionCapsule*)data;

    float distance = fabsf(direction->x);
    output->x = direction->x > 0.0f ? capsule->radius : -capsule->radius;
    output->y = 0.0f;
    output->z = 0.0f;

    int result = direction->x > 0.0f ? 0x1 : 0x8;

    for (int i = 1; i < 3; ++i) {
        float distanceCheck = fabsf(VECTOR3_AS_ARRAY(direction)[i]);

        if (distanceCheck > distance) {
            distance = distanceCheck;
            *output = gZeroVec;
            if (VECTOR3_AS_ARRAY(direction)[i] > 0.0f) {
                VECTOR3_AS_ARRAY(output)[i] = capsule->radius;
                result = 0x1 << i;
            } else {
                VECTOR3_AS_ARRAY(output)[i] = -capsule->radius;
                result = 0x8 << i;
            }
        }
    }

    float distanceCheck = fabsf(direction->x + direction->y + direction->z) * SQRT_3;

    if (distanceCheck > distance) {
        float scaledRadius = capsule->radius * SQRT_3;

        result = 64;

        if (output->x > 0.0f) {
            output->x = scaledRadius;
            result <<= 1;
        } else {
            output->x = -scaledRadius;
        }

        if (output->y > 0.0f) {
            output->y = scaledRadius;
            result <<= 2;
        } else {
            output->y = -scaledRadius;
        }

        if (output->z > 0.0f) {
            output->z = scaledRadius;
            result <<= 4;
        } else {
            output->z = -scaledRadius;
        }
    }

    if (direction->y < 0.0f) {
        vector3AddScaled(output, &basis->y, -capsule->extendDownward, output);
    }
    
    return result;
}

struct ColliderCallbacks gCollisionCapsuleCallbacks = {
    NULL, // TODO
    collisionCapsuleSolidMofI,
    collisionCapsuleBoundingBox,
    collisionCapsuleMinkowsiSum,
};
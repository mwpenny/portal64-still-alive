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
#define SQRT_2  0.707106781f

static struct Vector2 gUnitCircle[] = {
    {1.0f, 0.0f},
    {SQRT_2, SQRT_2},
    {0.0f, 1.0f},
    {-SQRT_2, SQRT_2},
    {-1.0f, 0.0f},
    {-SQRT_2, -SQRT_2},
    {0.0f, -1.0f},
    {SQRT_2, -SQRT_2},
};

#define OFFSET_IN_CIRCLE(current, amount)  (((current) + (amount)) & 0x7)

int collisionCapsuleMinkowsiSum(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output) {
    struct CollisionCapsule* capsule = (struct CollisionCapsule*)data;

    float directionY = vector3Dot(&basis->y, direction);

    if (directionY * directionY > 0.5f * vector3MagSqrd(direction)) {
        if (directionY > 0.0f) {
            vector3Scale(&basis->y, output, capsule->radius);
            return 0xFF;
        } else {
            vector3Scale(&basis->y, output, -capsule->radius - capsule->extendDownward);
            return 0xFF00;
        }
    } else {
        struct Vector2 horizontalBasis;

        horizontalBasis.x = vector3Dot(&basis->x, direction);
        horizontalBasis.y = vector3Dot(&basis->z, direction);

        int circleIndex = 0;
        float currentDot = vector2Dot(&gUnitCircle[0], &horizontalBasis);

        if (currentDot < 0.0f) {
            circleIndex = 4;
            currentDot = -currentDot;
        }

        for (int i = 2; i >=1; --i) {
            int nextIndex = OFFSET_IN_CIRCLE(circleIndex, i);
            int prevIndex = OFFSET_IN_CIRCLE(circleIndex, -i);

            float nextDot = vector2Dot(&gUnitCircle[nextIndex], &horizontalBasis);
            float prevDot = vector2Dot(&gUnitCircle[prevIndex], &horizontalBasis);

            if (nextDot > currentDot) {
                circleIndex = nextIndex;
                currentDot = nextDot;
            }

            if (prevDot > currentDot) {
                circleIndex = prevIndex;
                currentDot = prevDot;
            }
        }

        int result;

        if (circleIndex == 0) {
            result = 0x81;
        } else {
            result = 0xC0 >> (7 - circleIndex);
        }

        vector3Scale(&basis->x, output, gUnitCircle[circleIndex].x * capsule->radius);
        vector3AddScaled(output, &basis->z, gUnitCircle[circleIndex].y * capsule->radius, output);

        if (directionY < 0.0f) {
            vector3AddScaled(output, &basis->y, -capsule->extendDownward, output);
            result <<= 8;
        }

        return result;
    }
}

struct ColliderCallbacks gCollisionCapsuleCallbacks = {
    NULL, // TODO
    collisionCapsuleSolidMofI,
    collisionCapsuleBoundingBox,
    collisionCapsuleMinkowsiSum,
};
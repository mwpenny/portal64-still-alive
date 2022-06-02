#include "collision_point.h"

#include "contact_solver.h"
#include "collision_box.h"
#include "../math/transform.h"
#include "../math/mathf.h"

int collisionBoxCollidePoint(void* data, struct Transform* boxTransform, struct Vector3* point, struct ContactManifold* output) {
    struct Vector3 localSpace;
    transformPointInverseNoScale(boxTransform, point, &localSpace);

    struct Vector3 absLocalspace;
    vector3Abs(&localSpace, &absLocalspace);
    
    if (absLocalspace.x > 0.0f || absLocalspace.y > 0.0f || absLocalspace.z > 0.0f) {
        return 0;
    }

    struct CollisionBox* box = (struct CollisionBox*)data;

    float axisDistance = box->sideLength.x - absLocalspace.x;
    int axis = 0;

    float check = box->sideLength.y - absLocalspace.y;

    if (check < axisDistance) {
        axisDistance = check;
        axis = 1;
    }

    check = box->sideLength.z - absLocalspace.z;

    if (check < axisDistance) {
        axisDistance = check;
        axis = 2;
    }


    output->normal = gZeroVec;
    VECTOR3_AS_ARRAY(&output->normal)[axis] = signf(VECTOR3_AS_ARRAY(&localSpace)[0]);
    output->contactCount = 1;
    
    return 1;
}
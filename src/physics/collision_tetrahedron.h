#ifndef __COLLISION_TETRAHEDRON_H__
#define __COLLISION_TETRAHEDRON_H__

#include "collision.h"
#include "math/vector3.h"

struct CollisionTetrahedron {
    /*      .
           / \
          /   \
         /     \     Z
        /       \
       /_________\
            X
     */
    struct Vector3 dimensions;
};

extern struct ColliderCallbacks gCollisionTetrahedronCallbacks;

#endif

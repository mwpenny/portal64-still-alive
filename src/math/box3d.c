
#include "box3d.h"

int box3DContainsPoint(struct Box3D* box, struct Vector3* point) {
    return box->min.x < point->x &&
        box->min.y < point->y &&
        box->min.z < point->z &&
        box->max.x > point->x &&
        box->max.y > point->y &&
        box->max.z > point->z;
        
}

int box3DHasOverlap(struct Box3D* a, struct Box3D* b) {
    return a->min.x <= b->max.x && a->max.x >= b->min.x &&
        a->min.y <= b->max.y && a->max.y >= b->min.y &&
        a->min.z <= b->max.z && a->max.z >= b->min.z;
}

void box3DUnion(struct Box3D* a, struct Box3D* b, struct Box3D* out) {
    vector3Max(&a->max, &b->max, &out->max);
    vector3Min(&a->min, &b->min, &out->min);
}

void box3DUnionPoint(struct Box3D* a, struct Vector3* point, struct Box3D* out) {
    vector3Max(&a->max, point, &out->max);
    vector3Min(&a->min, point, &out->min);
}

void box3DSupportFunction(struct Box3D* box, struct Vector3* input, struct Vector3* output) {
    output->x = input->x > 0.0f ? box->max.x : box->min.x;
    output->y = input->y > 0.0f ? box->max.y : box->min.y;
    output->z = input->z > 0.0f ? box->max.z : box->min.z;
} 
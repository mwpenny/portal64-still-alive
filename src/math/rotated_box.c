#include "rotated_box.h"

void rotatedBoxTransform(struct Transform* transform, struct BoundingBoxs16* input, struct RotatedBox* output) {
    output->origin.x = input->minX;
    output->origin.y = input->minY;
    output->origin.z = input->minZ;

    output->sides[0].x = input->maxX - input->minX;
    output->sides[0].y = 0.0f;
    output->sides[0].z = 0.0f;

    output->sides[1].x = 0.0f;
    output->sides[1].y = input->maxY - input->minY;
    output->sides[1].z = 0.0f;

    output->sides[2].x = 0.0f;
    output->sides[2].y = 0.0f;
    output->sides[2].z = input->maxZ - input->minZ;

    transformPoint(transform, &output->origin, &output->origin);
    for (int axis = 0; axis < 3; ++axis) {
        quatMultVector(&transform->rotation, &output->sides[axis], &output->sides[axis]);
    }
}
#include "basis.h"

#include "../util/assert.h"

void basisFromQuat(struct Basis* basis, struct Quaternion* quat) {
    quatMultVector(quat, &gRight, &basis->x);
    quatMultVector(quat, &gUp, &basis->y);
    vector3Cross(&basis->x, &basis->y, &basis->z);
}

void basisRotate(struct Basis* basis, struct Vector3* input, struct Vector3* output) {
    __assert(input != output);

    vector3Scale(&basis->x, output, input->x);
    vector3AddScaled(output, &basis->y, input->y, output);
    vector3AddScaled(output, &basis->z, input->z, output);
}

void basisUnRotate(struct Basis* basis, struct Vector3* input, struct Vector3* output) {
    __assert(input != output);

    output->x = vector3Dot(&basis->x, input);
    output->y = vector3Dot(&basis->y, input);
    output->z = vector3Dot(&basis->z, input);
}
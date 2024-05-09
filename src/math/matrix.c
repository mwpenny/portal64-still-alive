#include "matrix.h"

#include <ultra64.h>
#include "defs.h"

void matrixPerspective(float matrix[4][4], unsigned short* perspNorm, float l, float r, float t, float b, float near, float far) {
	guMtxIdentF(matrix);

    matrix[0][0] = 2.0f * near / (r - l);
    matrix[1][1] = 2.0f * near / (t - b);
    matrix[2][0] = (r + l) / (r - l);
    matrix[2][1] = (t + b) / (t - b);
    matrix[2][2] = -(far + near) / (far - near);
    matrix[2][3] = -1;
    matrix[3][2] = -2.0f * far * near / (far - near);
    matrix[3][3] = 0.0f;

	if (perspNorm != (u16 *) NULL) {
	    if (near+far<=2.0) {
		    *perspNorm = (u16) 0xFFFF;
	    } else  {
		    *perspNorm = (u16) ((2.0*65536.0)/(near+far));
            if (*perspNorm<=0) {
                *perspNorm = (u16) 0x0001;
            }
	    }
	}
}

float matrixNormalizedZValue(float depth, float near, float far) {
    if (depth >= -near) {
        return -1.0f;
    }

    if (depth <= -far) {
        return 1.0f;
    }

    return (far * (depth + near) + 2.0 * far * near) / (depth * (far - near));
}

void matrixVec3Mul(float matrix[4][4], struct Vector3* input, struct Vector4* output) {
    output->x = matrix[0][0] * input->x + matrix[1][0] * input->y + matrix[2][0] * input->z + matrix[3][0];
    output->y = matrix[0][1] * input->x + matrix[1][1] * input->y + matrix[2][1] * input->z + matrix[3][1];
    output->z = matrix[0][2] * input->x + matrix[1][2] * input->y + matrix[2][2] * input->z + matrix[3][2];
    output->w = matrix[0][3] * input->x + matrix[1][3] * input->y + matrix[2][3] * input->z + matrix[3][3];
}

void matrixFromBasis(float matrix[4][4], struct Vector3* origin, struct Vector3* x, struct Vector3* y, struct Vector3* z) {
    matrix[0][0] = x->x;
    matrix[0][1] = x->y;
    matrix[0][2] = x->z;
    matrix[0][3] = 0.0f;

    matrix[1][0] = y->x;
    matrix[1][1] = y->y;
    matrix[1][2] = y->z;
    matrix[1][3] = 0.0f;

    matrix[2][0] = z->x;
    matrix[2][1] = z->y;
    matrix[2][2] = z->z;
    matrix[2][3] = 0.0f;

    matrix[3][0] = origin->x * SCENE_SCALE;
    matrix[3][1] = origin->y * SCENE_SCALE;
    matrix[3][2] = origin->z * SCENE_SCALE;
    matrix[3][3] = 1.0f;
}

#include "matrix.h"

#include <ultra64.h>

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
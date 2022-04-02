#ifndef __DEBUG_RENDERER_H__
#define __DEBUG_RENDERER_H__

#include "contact_solver.h"
#include "../graphics/renderstate.h"

void contactSolverDebugDraw(struct ContactSolver* contactSolver, struct RenderState* renderState);

#endif
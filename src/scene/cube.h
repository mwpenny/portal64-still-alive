#ifndef __SCENE_CUBE_H__
#define __SCENE_CUBE_H__

#include "../physics/rigid_body.h"
#include "../graphics/renderstate.h"

struct Cube {
    struct RigidBody rigidBody;
};

void cubeInit(struct Cube* cube);
void cubeUpdate(struct Cube* cube);
void cubeRender(struct Cube* cube, struct RenderState* renderState);

#endif
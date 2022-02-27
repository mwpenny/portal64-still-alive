#ifndef __SCENE_H__
#define __SCENE_H__

#include "camera.h"
#include "graphics/renderstate.h"
#include "shadow_renderer.h"
#include "shadow_map.h"
#include "point_light.h"
#include "../player/player.h"

struct Scene {
    struct Camera camera;
    struct Player player;
};

struct GraphicsTask;

void sceneInit(struct Scene* scene);
void sceneRender(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task);
void sceneUpdate(struct Scene* scene);

#endif
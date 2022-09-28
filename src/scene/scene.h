#ifndef __SCENE_H__
#define __SCENE_H__

#include "camera.h"
#include "graphics/renderstate.h"
#include "shadow_renderer.h"
#include "shadow_map.h"
#include "point_light.h"
#include "portal.h"
#include "../player/player.h"
#include "button.h"
#include "../decor/decor_object.h"
#include "./door.h"
#include "./fizzler.h"
#include "elevator.h"
#include "pedestal.h"
#include "signage.h"
#include "box_dropper.h"

struct Scene {
    struct Camera camera;
    struct Player player;
    struct Portal portals[2];
    struct Button* buttons;
    struct DecorObject** decor;
    struct Door* doors;
    struct Fizzler* fizzlers;
    struct Elevator* elevators;
    struct Pedestal* pedestals;
    struct Signage* signage;
    struct BoxDropper* boxDroppers;
    struct Vector3 freeCameraOffset;
    OSTime cpuTime;
    OSTime lastFrameStart;
    OSTime lastFrameTime;
    u8 buttonCount;
    u8 decorCount;
    u8 doorCount;
    u8 fizzlerCount;
    u8 elevatorCount;
    u8 pedestalCount;
    u8 signageCount;
    u8 boxDropperCount;
};

extern struct Scene gScene;

struct GraphicsTask;

void sceneInit(struct Scene* scene);
void sceneRender(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task);
void sceneUpdate(struct Scene* scene);

int sceneFirePortal(struct Scene* scene, struct Ray* ray, struct Vector3* playerUp, int portalIndex, int roomIndex);
void sceneClosePortal(struct Scene* scene, int portalIndex);

#endif
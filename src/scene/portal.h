#ifndef __PORTAL_H__
#define __PORTAL_H__

#include "../math/transform.h"
#include "../math/plane.h"
#include "../graphics/renderstate.h"
#include "camera.h"
#include "static_scene.h"


#define PORTAL_LOOP_SIZE    8

extern struct Vector3 gPortalOutlineUnscaled[PORTAL_LOOP_SIZE];

enum PortalFlags {
    PortalFlagsOddParity = (1 << 0),
};

struct Portal {
    struct Transform transform;
    enum PortalFlags flags;
};

struct RenderProps;

typedef void SceneRenderCallback(void* data, struct RenderProps* properties, struct RenderState* renderState);

struct RenderProps {
    struct Camera camera;
    float aspectRatio;
    Mtx* perspectiveMatrix;
    Vp* viewport;
    struct FrustrumCullingInformation cullingInfo;

    u16 perspectiveCorrect;
    short currentDepth;

    short minX;
    short minY;
    short maxX;
    short maxY;

};

#define STARTING_RENDER_DEPTH       1

void renderPropsInit(struct RenderProps* props, struct Camera* camera, float aspectRatio, struct RenderState* renderState);
void renderPropsNext(struct RenderProps* current, struct RenderProps* next, struct Transform* fromPortal, struct Transform* toPortal, struct RenderState* renderState);

void portalInit(struct Portal* portal, enum PortalFlags flags);

void portalRender(struct Portal* portal, struct Portal* otherPortal, struct RenderProps* props, SceneRenderCallback sceneRenderer, void* data, struct RenderState* renderState);

#endif
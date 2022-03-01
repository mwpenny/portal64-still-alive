#ifndef __PORTAL_H__
#define __PORTAL_H__

#include "../math/transform.h"
#include "../graphics/renderstate.h"
#include "camera.h"

enum PortalFlags {
    PortalFlagsOddParity = (1 << 0),
};

struct Portal {
    struct Transform transform;
    enum PortalFlags flags;
};

void portalInit(struct Portal* portal, enum PortalFlags flags);

void portalRender(struct Portal* portal, struct Portal* otherPortal, struct Camera* camera, struct RenderState* renderState);

#endif
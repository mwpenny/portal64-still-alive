#ifndef __SCENE_PORTAL_RENDER_H__
#define __SCENE_PORTAL_RENDER_H__

#include "../math/vector2s16.h"
#include "portal.h"
#include "../graphics/renderstate.h"

struct RenderProps;

void portalRenderScreenCover(struct Vector2s16* points, int pointCount, struct RenderProps* props, struct RenderState* renderState);
void portalDetermineTransform(struct Portal* portal, float portalTransform[4][4]);
void portalRenderCover(struct Portal* portal, float portalTransform[4][4], struct RenderState* renderState);

#endif
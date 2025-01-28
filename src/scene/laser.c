#include "laser.h"

#include "defs.h"
#include "dynamic_scene.h"
#include "graphics/color.h"

#include "codegen/assets/materials/static.h"

#define LASER_HALF_WIDTH 0.009375f

static struct Coloru8 laserColor = { 255, 0, 0, 128 };

static void laserRender(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct Laser* laser = (struct Laser*)data;

    // Determine screen-space up direction for billboard
    struct Vector3 laserForward;
    quatMultVector(&laser->parent->rotation, &gForward, &laserForward);

    struct Vector3 startPosition;
    quatMultVector(&laser->parent->rotation, &laser->parentOffset, &startPosition);
    vector3Add(&laser->parent->position, &startPosition, &startPosition);

    // TODO: dynamic end position
    struct Vector3 endPosition;
    vector3AddScaled(&startPosition, &laserForward, 10.0f, &endPosition);

    struct Vector3 tmp;
    struct Vector3 laserUp;
    vector3Sub(&startPosition, &fromView->position, &tmp);
    vector3Cross(&laserForward, &tmp, &laserUp);
    vector3Normalize(&laserUp, &laserUp);

    // Create vertices for quad
    Vtx* vertices = renderStateRequestVertices(renderScene->renderState, 4);
    Vtx* curr = vertices;

    struct Vector3* vertexOrigin;
    for (int i = 0; i < 4; ++i, ++curr) {
        vertexOrigin = (i >> 1) ? &endPosition : &startPosition;
        vector3AddScaled(
            vertexOrigin,
            &laserUp,
            (i & 1) ? -LASER_HALF_WIDTH : LASER_HALF_WIDTH,
            &tmp
        );

        curr->v.ob[0] = tmp.x * SCENE_SCALE;
        curr->v.ob[1] = tmp.y * SCENE_SCALE;
        curr->v.ob[2] = tmp.z * SCENE_SCALE;

        curr->v.flag = 0;
        curr->v.tc[0] = 0;
        curr->v.tc[1] = 0;

        curr->v.cn[0] = laserColor.r;
        curr->v.cn[1] = laserColor.g;
        curr->v.cn[2] = laserColor.b;
        curr->v.cn[3] = laserColor.a;
    }

    // Render quad
    Gfx* displayList = renderStateAllocateDLChunk(renderScene->renderState, 3);
    Gfx* dl = displayList;

    gSPVertex(dl++, vertices, 4, 0);
    gSP1Quadrangle(
        dl++,
        0, 1, 3, 2,
        0
    );
    gSPEndDisplayList(dl++);

    // TODO: proper position for geometry sorting
    renderSceneAdd(renderScene, displayList, NULL, LASER_INDEX, &laser->parent->position, NULL);
}

void laserInit(struct Laser* laser, struct Transform* parent, struct Vector3* offset) {
    laser->parent = parent;
    laser->parentOffset = *offset;

    // TODO (culling): proper position and radius, set room flags
    laser->dynamicId = dynamicSceneAddViewDependant(laser, laserRender, &laser->parent->position, 20.0f);
}

void laserUpdate(struct Laser* laser) {
    // TODO: update room, pass through portals, etc.
}

void laserRemove(struct Laser* laser) {
    // TODO
}

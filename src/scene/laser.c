#include "laser.h"

#include <assert.h>

#include "defs.h"
#include "dynamic_scene.h"
#include "graphics/color.h"
#include "physics/collision_scene.h"

#include "codegen/assets/materials/static.h"

#define LASER_HALF_WIDTH       0.009375f

#define LASER_COLLISION_LAYERS (COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_TURRET_SHOTS)

static struct Coloru8 laserColor = { 255, 0, 0, 100 };

static void laserBeamBuildVtx(Vtx* vtx, struct LaserBeam* beam, struct Vector3* cameraPosition) {
    // Determine screen-space up direction for billboard
    struct Vector3 tmp;
    struct Vector3 laserUp;
    vector3Sub(&beam->startPosition.origin, cameraPosition, &tmp);
    vector3Cross(&beam->startPosition.dir, &tmp, &laserUp);
    vector3Normalize(&laserUp, &laserUp);

    // Create vertices for quad
    struct Vector3* vertexOrigin;
    for (int i = 0; i < 4; ++i, ++vtx) {
        vertexOrigin = (i >> 1) ? &beam->endPosition : &beam->startPosition.origin;
        vector3AddScaled(
            vertexOrigin,
            &laserUp,
            (i & 1) ? -LASER_HALF_WIDTH : LASER_HALF_WIDTH,
            &tmp
        );

        vtx->v.ob[0] = tmp.x * SCENE_SCALE;
        vtx->v.ob[1] = tmp.y * SCENE_SCALE;
        vtx->v.ob[2] = tmp.z * SCENE_SCALE;

        vtx->v.flag = 0;
        vtx->v.tc[0] = 0;
        vtx->v.tc[1] = 0;

        vtx->v.cn[0] = laserColor.r;
        vtx->v.cn[1] = laserColor.g;
        vtx->v.cn[2] = laserColor.b;
        vtx->v.cn[3] = laserColor.a;
    }
}

static void laserRender(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct Laser* laser = (struct Laser*)data;
    if (laser->beamCount == 0) {
        return;
    }

    // Build quads
    Vtx* vertices = renderStateRequestVertices(renderScene->renderState, laser->beamCount * 4);
    Vtx* curr = vertices;
    for (short i = 0; i < laser->beamCount; ++i, curr += 4) {
        laserBeamBuildVtx(curr, &laser->beams[i], &fromView->position);
    }

    // Render quads
    Gfx* displayList = renderStateAllocateDLChunk(renderScene->renderState, laser->beamCount + 2);
    Gfx* dl = displayList;

    assert(LASER_MAX_BEAMS <= 8);
    gSPVertex(dl++, vertices, 4 * laser->beamCount, 0);

    for (short i = 0; i < laser->beamCount; ++i) {
        short relativeVertex = i << 2;
        gSP1Quadrangle(
            dl++,
            relativeVertex,
            relativeVertex + 1,
            relativeVertex + 3,
            relativeVertex + 2,
            0
        );
    }

    gSPEndDisplayList(dl++);

    renderSceneAdd(renderScene, displayList, NULL, LASER_INDEX, &laser->parent->position, NULL);
}

void laserInit(struct Laser* laser, struct Transform* parent, struct Vector3* offset) {
    laser->parent = parent;
    laser->parentOffset = *offset;
    laser->beamCount = 0;

    // The laser could traverse multiple rooms and pass through portals.
    //
    // To avoid a dynamic object per beam, this single object renders them all,
    // and so efficiently deriving a meaningful culling radius is difficult.
    // Given typical turret gameplay and the low rendering cost (each beam is
    // just a quad), rely solely on room flags.
    laser->dynamicId = dynamicSceneAddViewDependant(laser, laserRender, &laser->parent->position, 1000000.0f);
}

void laserUpdate(struct Laser* laser, int currentRoom) {
    struct Ray startPosition = {
        .origin = laser->parentOffset,
        .dir    = gForward
    };
    struct Transform startTransform = *laser->parent;
    uint64_t beamRooms = 0;

    laser->beamCount = 0;

    for (short i = 0; i < LASER_MAX_BEAMS; ++i) {
        struct LaserBeam* beam = &laser->beams[i];
        rayTransform(&startTransform, &startPosition, &startPosition);
        beam->startPosition = startPosition;

        struct RaycastHit hit;
        if (!collisionSceneRaycast(
                &gCollisionScene,
                currentRoom,
                &beam->startPosition,
                LASER_COLLISION_LAYERS,
                1000000.0f,
                0,
                &hit)
        ) {
            break;
        }

        beam->endPosition = hit.at;
        beamRooms |= hit.passedRooms;
        ++laser->beamCount;

        int touchingPortals = collisionSceneIsTouchingPortal(&hit.at, &hit.normal);
        if (!touchingPortals) {
            break;
        } else {
            int portalIndex = (touchingPortals & RigidBodyIsTouchingPortalA) ? 0 : 1;
            collisionSceneGetPortalTransform(portalIndex, &startTransform);

            startPosition.origin = beam->endPosition;
            currentRoom = gCollisionScene.portalRooms[1 - portalIndex];
        }
    }

    dynamicSceneSetRoomFlags(laser->dynamicId, beamRooms);
}

void laserRemove(struct Laser* laser) {
    // TODO
}

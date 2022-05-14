
#include "scene.h"

#include "defs.h"
#include "graphics/graphics.h"
#include "models/models.h"
#include "materials/shadow_caster.h"
#include "materials/subject.h"
#include "materials/light.h"
#include "materials/point_light_rendered.h"
#include "util/time.h"
#include "sk64/skelatool_defs.h"
#include "controls/controller.h"
#include "shadow_map.h"
#include "../physics/point_constraint.h"
#include "../physics/debug_renderer.h"
#include "../controls/controller.h"
#include "../physics/collision_scene.h"
#include "../levels/static_render.h"
#include "../levels/levels.h"
#include "../scene/portal_surface.h"
#include "../math/mathf.h"
#include "./hud.h"
#include "dynamic_scene.h"

struct Vector3 gStartPosition = {5.0f, 1.2f, -5.0f};
struct Vector3 gPortalGunOffset = {0.100957, -0.113587, -0.28916};
struct Vector3 gPortalGunForward = {0.1f, -0.1f, 1.0f};
struct Vector3 gPortalGunUp = {0.0f, 1.0f, 0.0f};

Lights1 gSceneLights = gdSPDefLights1(128, 128, 128, 128, 128, 128, 0, 127, 0);

void sceneInit(struct Scene* scene) {
    cameraInit(&scene->camera, 45.0f, 0.125f * SCENE_SCALE, 80.0f * SCENE_SCALE);
    playerInit(&scene->player);

    scene->player.body.transform.position = gStartPosition;
    quatAxisAngle(&gUp, M_PI, &scene->player.body.transform.rotation);

    portalInit(&scene->portals[0], 0);
    portalInit(&scene->portals[1], PortalFlagsOddParity);
    gCollisionScene.portalTransforms[0] = &scene->portals[0].transform;
    gCollisionScene.portalTransforms[1] = &scene->portals[1].transform;

    scene->portals[0].transform.position.x = 5.0f;
    scene->portals[0].transform.position.y = 1.0f;
    scene->portals[0].transform.position.z = -0.1f;

    scene->portals[1].transform.position.x = 0.1f;
    scene->portals[1].transform.position.y = 1.0f;
    scene->portals[1].transform.position.z = -6.0f;


    quatAxisAngle(&gUp, M_PI * 0.5f, &scene->portals[1].transform.rotation);

    cubeInit(&scene->cube);

    scene->cube.rigidBody.transform.position.x = 5.0f;
    scene->cube.rigidBody.transform.position.y = 1.0f;
    scene->cube.rigidBody.transform.position.z = -1.0f;

    quatAxisAngle(&gRight, M_PI * 0.125f, &scene->cube.rigidBody.transform.rotation);
    scene->cube.rigidBody.angularVelocity = gOneVec;
}

void sceneRenderWithProperties(void* data, struct RenderProps* properties, struct RenderState* renderState) {
    struct Scene* scene = (struct Scene*)data;

    int closerPortal = vector3DistSqrd(&properties->camera.transform.position, &scene->portals[0].transform.position) > vector3DistSqrd(&properties->camera.transform.position, &scene->portals[1].transform.position) ? 0 : 1;

    if (properties->fromPortalIndex != closerPortal) {
        portalRender(&scene->portals[closerPortal], &scene->portals[1 - closerPortal], properties, sceneRenderWithProperties, data, renderState);
    }
    if (properties->fromPortalIndex != 1 - closerPortal) {
        portalRender(&scene->portals[1 - closerPortal], &scene->portals[closerPortal], properties, sceneRenderWithProperties, data, renderState);
    }

    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);

    staticRender(&properties->cullingInfo, renderState);

    dynamicSceneRender(renderState, 0);
}

#define SOLID_COLOR        0, 0, 0, ENVIRONMENT, 0, 0, 0, ENVIRONMENT

void sceneRenderPerformanceMetrics(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task) {
    gDPSetCycleType(renderState->dl++, G_CYC_1CYCLE);
    gDPSetFillColor(renderState->dl++, (GPACK_RGBA5551(0, 0, 0, 1) << 16 | GPACK_RGBA5551(0, 0, 0, 1)));
    gDPSetCombineMode(renderState->dl++, SOLID_COLOR, SOLID_COLOR);
    gDPSetEnvColor(renderState->dl++, 32, 32, 32, 255);
    gSPTextureRectangle(renderState->dl++, 32 << 2, 32 << 2, (32 + 256) << 2, (32 + 16) << 2, 0, 0, 0, 1, 1);
    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, 32, 255, 32, 255);
    gSPTextureRectangle(renderState->dl++, 33 << 2, 33 << 2, (32 + 254 * scene->cpuTime / scene->lastFrameTime) << 2, (32 + 14) << 2, 0, 0, 0, 1, 1);
    gDPPipeSync(renderState->dl++);
}

void sceneRenderPortalGun(struct Scene* scene, struct RenderState* renderState) {
    struct Transform gunTransform;
    transformPoint(&scene->player.body.transform, &gPortalGunOffset, &gunTransform.position);
    struct Quaternion relativeRotation;
    quatLook(&gPortalGunForward, &gPortalGunUp, &relativeRotation);
    quatMultiply(&scene->player.body.transform.rotation, &relativeRotation, &gunTransform.rotation);
    gunTransform.scale = gOneVec;
    Mtx* matrix = renderStateRequestMatrices(renderState, 1);
    transformToMatrixL(&gunTransform, matrix, SCENE_SCALE);

    gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    gSPDisplayList(renderState->dl++, v_portal_gun_gfx);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}

void sceneRender(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPSetLights1(renderState->dl++, gSceneLights);
    
    struct RenderProps renderProperties;

    renderPropsInit(&renderProperties, &scene->camera, (float)SCREEN_WD / (float)SCREEN_HT, renderState);

    renderProperties.camera = scene->camera;

    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);

    dynamicSceneRender(renderState, 1);

    sceneRenderWithProperties(scene, &renderProperties, renderState);

    sceneRenderPortalGun(scene, renderState);

    gDPPipeSync(renderState->dl++);
    gDPSetRenderMode(renderState->dl++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gSPGeometryMode(renderState->dl++, G_ZBUFFER | G_LIGHTING | G_CULL_BOTH, G_SHADE);

    hudRender(renderState);

    // sceneRenderPerformanceMetrics(scene, renderState, task);

    // contactSolverDebugDraw(&gContactSolver, renderState);
}

void sceneCheckPortals(struct Scene* scene) {
    struct Ray raycastRay;
    struct Vector3 playerUp;
    raycastRay.origin = scene->player.body.transform.position;
    vector3Negate(&gForward, &raycastRay.dir);
    quatMultVector(&scene->player.body.transform.rotation, &raycastRay.dir, &raycastRay.dir);
    quatMultVector(&scene->player.body.transform.rotation, &gUp, &playerUp);

    if (controllerGetButtonDown(0, Z_TRIG)) {
        sceneFirePortal(scene, &raycastRay, &playerUp, 0);
    }

    if (controllerGetButtonDown(0, R_TRIG)) {
        sceneFirePortal(scene, &raycastRay, &playerUp, 1);
    }
}

void sceneUpdate(struct Scene* scene) {
    OSTime frameStart = osGetTime();
    scene->lastFrameTime = frameStart - scene->lastFrameStart;

    playerUpdate(&scene->player, &scene->camera.transform);
    sceneCheckPortals(scene);

    cubeUpdate(&scene->cube);
    
    collisionSceneUpdateDynamics();

    scene->cpuTime = osGetTime() - frameStart;
    scene->lastFrameStart = frameStart;
}

int sceneFirePortal(struct Scene* scene, struct Ray* ray, struct Vector3* playerUp, int portalIndex) {
    struct RaycastHit hit;

    if (!collisionSceneRaycast(&gCollisionScene, ray, 1000000.0f, 0, &hit)) {
        return 0;
    }

    int quadIndex = levelQuadIndex(hit.object);

    if (quadIndex == -1) {
        return 0;
    }

    struct Transform portalLocation;

    struct Vector3 hitDirection = hit.normal;

    if (portalIndex == 1) {
        vector3Negate(&hitDirection, &hitDirection);
    }

    portalLocation.position = hit.at;
    portalLocation.scale = gOneVec;
    if (fabsf(hit.normal.y) < 0.8) {
        quatLook(&hitDirection, &gUp, &portalLocation.rotation);
    } else {
        quatLook(&hitDirection, playerUp, &portalLocation.rotation);
    }

    return sceneOpenPortal(scene, &portalLocation, portalIndex, quadIndex);
}

int sceneOpenPortal(struct Scene* scene, struct Transform* at, int portalIndex, int quadIndex) {
    struct PortalSurfaceMapping surfaceMapping = gCurrentLevel->portalSurfaceMapping[quadIndex];

    for (int i = surfaceMapping.minPortalIndex; i < surfaceMapping.maxPortalIndex; ++i) {
        if (portalSurfaceGenerate(&gCurrentLevel->portalSurfaces[i], at, NULL, NULL)) {
            struct Vector3 portalForward;
            quatMultVector(&at->rotation, &gForward, &portalForward);
            // TODO remove once there is a hole in the wall
            vector3AddScaled(&at->position, &portalForward, (portalIndex == 0) ? -0.1f : 0.1f, &at->position);
            
            scene->portals[portalIndex].transform = *at;
            gCollisionScene.portalTransforms[portalIndex] = &scene->portals[portalIndex].transform;
            return 1;
        }
    }

    return 0;
}
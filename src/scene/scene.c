
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
#include "../audio/soundplayer.h"
#include "../audio/clips.h"
#include "../levels/cutscene_runner.h"
#include "../util/memory.h"
#include "../decor/decor_object_list.h"
#include "signals.h"

struct Vector3 gPortalGunOffset = {0.120957, -0.113587, -0.20916};
struct Vector3 gPortalGunForward = {0.1f, -0.1f, 1.0f};
struct Vector3 gPortalGunUp = {0.0f, 1.0f, 0.0f};

Lights1 gSceneLights = gdSPDefLights1(128, 128, 128, 128, 128, 128, 0, 127, 0);

void sceneUpdateListeners(struct Scene* scene);

void sceneInit(struct Scene* scene) {
    signalsInit(1);

    cameraInit(&scene->camera, 70.0f, 0.125f * SCENE_SCALE, 40.0f * SCENE_SCALE);
    playerInit(&scene->player, levelGetLocation(gCurrentLevel->startLocation));
    sceneUpdateListeners(scene);

    portalInit(&scene->portals[0], 0);
    portalInit(&scene->portals[1], PortalFlagsOddParity);

    scene->buttonCount = gCurrentLevel->buttonCount;
    scene->buttons = malloc(sizeof(struct Button) * scene->buttonCount);

    for (int i = 0; i < scene->buttonCount; ++i) {
        buttonInit(&scene->buttons[i], &gCurrentLevel->buttons[i]);
    }

    scene->decorCount = gCurrentLevel->decorCount;
    scene->decor = malloc(sizeof(struct DecorObject*) * scene->decorCount);

    for (int i = 0; i < scene->decorCount; ++i) {
        struct DecorDefinition* decorDef = &gCurrentLevel->decor[i];
        struct Transform decorTransform;
        decorTransform.position = decorDef->position;
        decorTransform.rotation = decorDef->rotation;
        decorTransform.scale = gOneVec;
        scene->decor[i] = decorObjectNew(decorObjectDefinitionForId(decorDef->decorId), &decorTransform, decorDef->roomIndex);
    }

    scene->doorCount = gCurrentLevel->doorCount;
    scene->doors = malloc(sizeof(struct Door) * scene->doorCount);
    for (int i = 0; i < scene->doorCount; ++i) {
        doorInit(&scene->doors[i], &gCurrentLevel->doors[i], &gCurrentLevel->world);
    }

    scene->fizzlerCount = gCurrentLevel->fizzlerCount;
    scene->fizzlers = malloc(sizeof(struct Fizzler) * scene->fizzlerCount);
    for (int i = 0; i < scene->fizzlerCount; ++i) {
        struct FizzlerDefinition* fizzlerDef = &gCurrentLevel->fizzlers[i];

        struct Transform fizzlerTransform;
        fizzlerTransform.position = fizzlerDef->position;
        fizzlerTransform.rotation = fizzlerDef->rotation;
        fizzlerTransform.scale = gOneVec;
        fizzlerInit(&scene->fizzlers[i], &fizzlerTransform, fizzlerDef->width, fizzlerDef->height, fizzlerDef->roomIndex);
    }

    scene->elevatorCount = gCurrentLevel->elevatorCount;
    scene->elevators = malloc(sizeof(struct Elevator) * scene->elevatorCount);
    for (int i = 0; i < scene->elevatorCount; ++i) {
        elevatorInit(&scene->elevators[i], &gCurrentLevel->elevators[i]);
    }
}

void sceneRenderWithProperties(void* data, struct RenderProps* properties, struct RenderState* renderState) {
    struct Scene* scene = (struct Scene*)data;

    u64 visibleRooms = 0;
    staticRenderDetermineVisibleRooms(&properties->cullingInfo, properties->fromRoom, &visibleRooms);

    int closerPortal = vector3DistSqrd(&properties->camera.transform.position, &scene->portals[0].transform.position) > vector3DistSqrd(&properties->camera.transform.position, &scene->portals[1].transform.position) ? 0 : 1;
    int otherPortal = 1 - closerPortal;

    for (int i = 0; i < 2; ++i) {
        if (gCollisionScene.portalTransforms[closerPortal] && 
            properties->fromPortalIndex != closerPortal && 
            staticRenderIsRoomVisible(visibleRooms, gCollisionScene.portalRooms[closerPortal])) {
            portalRender(
                &scene->portals[closerPortal], 
                gCollisionScene.portalTransforms[otherPortal] ? &scene->portals[otherPortal] : NULL, 
                properties, 
                sceneRenderWithProperties, 
                data, 
                renderState
            );
        }

        closerPortal = 1 - closerPortal;
        otherPortal = 1 - otherPortal;
    }

    if (controllerGetButton(1, A_BUTTON) && properties->currentDepth == 2) {
        return;
    }

    staticRender(&properties->camera.transform, &properties->cullingInfo, visibleRooms, renderState);
}

#define SOLID_COLOR        0, 0, 0, ENVIRONMENT, 0, 0, 0, ENVIRONMENT

void sceneRenderPerformanceMetrics(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task) {
    if (!scene->lastFrameTime) {
        return;
    }

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
    transformPoint(&scene->player.lookTransform, &gPortalGunOffset, &gunTransform.position);
    struct Quaternion relativeRotation;
    quatLook(&gPortalGunForward, &gPortalGunUp, &relativeRotation);
    quatMultiply(&scene->player.lookTransform.rotation, &relativeRotation, &gunTransform.rotation);
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

    renderPropsInit(&renderProperties, &scene->camera, (float)SCREEN_WD / (float)SCREEN_HT, renderState, scene->player.body.currentRoom);

    renderProperties.camera = scene->camera;

    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);

    dynamicSceneRenderTouchingPortal(&scene->camera.transform, &renderProperties.cullingInfo, renderState);

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
    raycastRay.origin = scene->player.lookTransform.position;
    vector3Negate(&gForward, &raycastRay.dir);
    quatMultVector(&scene->player.lookTransform.rotation, &raycastRay.dir, &raycastRay.dir);
    quatMultVector(&scene->player.lookTransform.rotation, &gUp, &playerUp);

    if (controllerGetButtonDown(0, Z_TRIG)) {
        sceneFirePortal(scene, &raycastRay, &playerUp, 0, scene->player.body.currentRoom);
        soundPlayerPlay(soundsPortalgunShoot[0], 1.0f, 1.0f, NULL);
    }

    if (controllerGetButtonDown(0, R_TRIG | L_TRIG)) {
        sceneFirePortal(scene, &raycastRay, &playerUp, 1, scene->player.body.currentRoom);
        soundPlayerPlay(soundsPortalgunShoot[1], 1.0f, 1.0f, NULL);
    }

    if (scene->player.body.flags & RigidBodyFizzled) {
        sceneClosePortal(scene, 0);
        sceneClosePortal(scene, 1);
        scene->player.body.flags &= ~RigidBodyFizzled;
    }

    int isOpen = collisionSceneIsPortalOpen();

    portalUpdate(&scene->portals[0], isOpen);
    portalUpdate(&scene->portals[1], isOpen);

    portalCheckForHoles(scene->portals);
}

void sceneUpdatePortalListener(struct Scene* scene, int portalIndex, int listenerIndex) {
    struct Transform otherInverse;
    transformInvert(&scene->portals[1 - portalIndex].transform, &otherInverse);
    struct Transform portalCombined;
    transformConcat(&scene->portals[portalIndex].transform, &otherInverse, &portalCombined);

    struct Transform relativeTransform;
    transformConcat(&portalCombined, &scene->player.lookTransform, &relativeTransform);

    soundListenerUpdate(&relativeTransform.position, &relativeTransform.rotation, listenerIndex);
}

void sceneUpdateListeners(struct Scene* scene) {
    soundListenerUpdate(&scene->player.lookTransform.position, &scene->player.lookTransform.rotation, 0);

    if (collisionSceneIsPortalOpen()) {
        soundListenerSetCount(3);
        sceneUpdatePortalListener(scene, 0, 1);
        sceneUpdatePortalListener(scene, 1, 2);
    } else {
        soundListenerSetCount(1);
    }
}

void sceneUpdate(struct Scene* scene) {
    OSTime frameStart = osGetTime();
    scene->lastFrameTime = frameStart - scene->lastFrameStart;

    signalsReset();

    playerUpdate(&scene->player, &scene->camera.transform);
    sceneUpdateListeners(scene);
    sceneCheckPortals(scene);
    
    for (int i = 0; i < scene->buttonCount; ++i) {
        buttonUpdate(&scene->buttons[i]);
    }

    for (int i = 0; i < scene->doorCount; ++i) {
        doorUpdate(&scene->doors[i]);
    }

    for (int i = 0; i < scene->decorCount; ++i) {
        decorObjectUpdate(scene->decor[i]);
    }

    for (int i = 0; i < scene->fizzlerCount; ++i) {
        fizzlerUpdate(&scene->fizzlers[i]);
    }
    
    for (int i = 0; i < scene->elevatorCount; ++i) {
        elevatorUpdate(&scene->elevators[i], &scene->player);
    }
    
    collisionSceneUpdateDynamics();

    levelCheckTriggers(&scene->player.lookTransform.position);
    cutscenesUpdate();

    scene->cpuTime = osGetTime() - frameStart;
    scene->lastFrameStart = frameStart;
}

int sceneOpenPortal(struct Scene* scene, struct Transform* at, int portalIndex, int quadIndex, int roomIndex) {
    struct PortalSurfaceMappingRange surfaceMapping = gCurrentLevel->portalSurfaceMappingRange[quadIndex];

    for (int indexIndex = surfaceMapping.minPortalIndex; indexIndex < surfaceMapping.maxPortalIndex; ++indexIndex) {
        int surfaceIndex = gCurrentLevel->portalSurfaceMappingIndices[indexIndex];

        struct PortalSurface* existingSurface = portalSurfaceGetOriginalSurface(surfaceIndex, portalIndex);

        struct Portal* portal = &scene->portals[portalIndex];

        if (portalAttachToSurface(portal, existingSurface, surfaceIndex, at)) {
            soundPlayerPlay(soundsPortalOpen2, 1.0f, 1.0f, &at->position);
            
            portal->transform = *at;
            portal->roomIndex = roomIndex;
            portal->scale = 0.0f;
            gCollisionScene.portalTransforms[portalIndex] = &portal->transform;
            gCollisionScene.portalRooms[portalIndex] = roomIndex;

            if (collisionSceneIsPortalOpen()) {
                // the second portal is fully transparent right away
                portal->opacity = 0.0f;
            }

            contactSolverCheckPortalContacts(&gContactSolver, &gCurrentLevel->collisionQuads[quadIndex]);
            return 1;
        }
    }

    return 0;
}

int sceneFirePortal(struct Scene* scene, struct Ray* ray, struct Vector3* playerUp, int portalIndex, int roomIndex) {
    struct RaycastHit hit;

    if (!collisionSceneRaycast(&gCollisionScene, roomIndex, ray, COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_PORTAL, 1000000.0f, 0, &hit)) {
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

    return sceneOpenPortal(scene, &portalLocation, portalIndex, quadIndex, hit.roomIndex);
}

void sceneClosePortal(struct Scene* scene, int portalIndex) {
    if (gCollisionScene.portalTransforms[portalIndex]) {
        soundPlayerPlay(soundsPortalFizzle, 1.0f, 1.0f, &gCollisionScene.portalTransforms[portalIndex]->position);
        gCollisionScene.portalTransforms[portalIndex] = NULL;
        scene->portals[portalIndex].flags |= PortalFlagsNeedsNewHole;
        scene->portals[portalIndex].portalSurfaceIndex = -1;
    }
}
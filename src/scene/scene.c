
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

    cameraInit(&scene->camera, 70.0f, 0.125f * SCENE_SCALE, 30.0f * SCENE_SCALE);

    struct Location* startLocation = levelGetLocation(gCurrentLevel->startLocation);
    struct Location combinedLocation;
    struct Vector3 startVelocity;
    combinedLocation.roomIndex = startLocation->roomIndex;
    transformConcat(&startLocation->transform, levelRelativeTransform(), &combinedLocation.transform);
    quatMultVector(&startLocation->transform.rotation, levelRelativeVelocity(), &startVelocity);

    playerInit(&scene->player, &combinedLocation, &startVelocity);
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

    scene->pedestalCount = gCurrentLevel->pedestalCount;
    scene->pedestals = malloc(sizeof(struct Pedestal) * scene->pedestalCount);
    for (int i = 0; i < scene->pedestalCount; ++i) {
        pedestalInit(&scene->pedestals[i], &gCurrentLevel->pedestals[i]);
    }

    scene->signageCount = gCurrentLevel->signageCount;
    scene->signage = malloc(sizeof(struct Signage) * scene->signageCount);
    for (int i = 0; i < scene->signageCount; ++i) {
        signageInit(&scene->signage[i], &gCurrentLevel->signage[i]);
    }

    scene->boxDropperCount = gCurrentLevel->boxDropperCount;
    scene->boxDroppers = malloc(sizeof(struct BoxDropper) * scene->boxDropperCount);
    for (int i = 0; i < scene->boxDropperCount; ++i) {
        boxDropperInit(&scene->boxDroppers[i], &gCurrentLevel->boxDroppers[i]);
    }

    scene->freeCameraOffset = gZeroVec;
}

void sceneRenderWithProperties(void* data, struct RenderProps* properties, struct RenderState* renderState) {
    struct Scene* scene = (struct Scene*)data;

    u64 visibleRooms = 0;
    staticRenderDetermineVisibleRooms(&properties->cullingInfo, properties->fromRoom, &visibleRooms);

    int closerPortal = vector3DistSqrd(&properties->camera.transform.position, &scene->portals[0].transform.position) < vector3DistSqrd(&properties->camera.transform.position, &scene->portals[1].transform.position) ? 0 : 1;
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

LookAt gLookAt = gdSPDefLookAt(127, 0, 0, 0, 127, 0);

void sceneRender(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPSetLights1(renderState->dl++, gSceneLights);
    LookAt* lookAt = renderStateRequestLookAt(renderState);
    *lookAt = gLookAt;
    gSPLookAt(renderState->dl++, lookAt);
    
    struct RenderProps renderProperties;

    renderPropsInit(&renderProperties, &scene->camera, (float)SCREEN_WD / (float)SCREEN_HT, renderState, scene->player.body.currentRoom);

    renderProperties.camera = scene->camera;

    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);

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

struct Transform gRelativeElevatorTransform = {
    {0.0f, 0.0f, 0.0f},
    {0.5f, -0.5f, -0.5f, 0.5f},
    {1.0f, 1.0f, 1.0f},
};

#define FREE_CAM_VELOCITY   2.0f

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

    signalsEvaluateSignals(gCurrentLevel->signalOperators, gCurrentLevel->signalOperatorCount);

    for (int i = 0; i < scene->doorCount; ++i) {
        doorUpdate(&scene->doors[i]);
    }

    int decorWriteIndex = 0;

    for (int i = 0; i < scene->decorCount; ++i) {
        if (!decorObjectUpdate(scene->decor[i])) {
            decorObjectDelete(scene->decor[i]);
            continue;;
        }

        if (decorWriteIndex != i) {
            scene->decor[decorWriteIndex] = scene->decor[i];
        }

        ++decorWriteIndex;
    }

    scene->decorCount = decorWriteIndex;

    for (int i = 0; i < scene->fizzlerCount; ++i) {
        fizzlerUpdate(&scene->fizzlers[i]);
    }
    
    for (int i = 0; i < scene->elevatorCount; ++i) {
        int teleportTo = elevatorUpdate(&scene->elevators[i], &scene->player);

        if (teleportTo != -1) {
            if (teleportTo >= scene->elevatorCount) {
                struct Transform exitInverse;
                struct Transform fullExitTransform;

                transformConcat(&scene->elevators[i].rigidBody.transform, &gRelativeElevatorTransform, &fullExitTransform);

                transformInvert(&fullExitTransform, &exitInverse);
                struct Transform relativeExit;
                struct Vector3 relativeVelocity;

                transformConcat(&exitInverse, &scene->player.lookTransform, &relativeExit);
                quatMultVector(&exitInverse.rotation, &scene->player.body.velocity, &relativeVelocity);
                levelQueueLoad(NEXT_LEVEL, &relativeExit, &relativeVelocity);
            } else {
                rigidBodyTeleport(
                    &scene->player.body,
                    &scene->elevators[i].rigidBody.transform,
                    &scene->elevators[teleportTo].rigidBody.transform,
                    scene->elevators[teleportTo].roomIndex
                );
            }
        }
    }

    for (int i = 0; i < scene->pedestalCount; ++i) {
        pedestalUpdate(&scene->pedestals[i]);
    }

    for (int i = 0; i < scene->boxDropperCount; ++i) {
        boxDropperUpdate(&scene->boxDroppers[i]);
    }
    
    collisionSceneUpdateDynamics();

    levelCheckTriggers(&scene->player.lookTransform.position);
    cutscenesUpdate();

    scene->cpuTime = osGetTime() - frameStart;
    scene->lastFrameStart = frameStart;

    OSContPad* freecam = controllersGetControllerData(1);


    struct Vector3 lookDir;
    struct Vector3 rightDir;

    playerGetMoveBasis(&scene->camera.transform, &lookDir, &rightDir);

    if (freecam->stick_y) {
        if (controllerGetButton(1, Z_TRIG)) {
            vector3AddScaled(
                &scene->freeCameraOffset, 
                &lookDir, 
                -freecam->stick_y * (FREE_CAM_VELOCITY * FIXED_DELTA_TIME / 80.0f), 
                &scene->freeCameraOffset
            );
        } else {
            scene->freeCameraOffset.y += freecam->stick_y * (FREE_CAM_VELOCITY * FIXED_DELTA_TIME / 80.0f);
        }
    }

    if (freecam->stick_x) {
        vector3AddScaled(
            &scene->freeCameraOffset, 
            &rightDir, 
            freecam->stick_x * (FREE_CAM_VELOCITY * FIXED_DELTA_TIME / 80.0f), 
            &scene->freeCameraOffset
        );
    }

    if (controllerGetButtonDown(1, START_BUTTON)) {
        scene->freeCameraOffset = gZeroVec;
    }

    vector3Add(&scene->camera.transform.position, &scene->freeCameraOffset, &scene->camera.transform.position);

    if (controllerGetButtonDown(1, L_TRIG)) {
        struct Transform identityTransform;
        transformInitIdentity(&identityTransform);
        identityTransform.position.y = 1.0f;
        levelQueueLoad(NEXT_LEVEL, &identityTransform, &gZeroVec);
    }
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
        struct Vector3 upDir;

        if (ray->dir.y > 0.0f) {
            vector3Negate(playerUp, &upDir);
        } else {
            upDir = *playerUp;
        }

        quatLook(&hitDirection, &upDir, &portalLocation.rotation);
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
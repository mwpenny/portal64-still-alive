
#include "scene.h"

#include "defs.h"
#include "graphics/graphics.h"
#include "materials/shadow_caster.h"
#include "materials/subject.h"
#include "materials/light.h"
#include "materials/point_light_rendered.h"
#include "util/time.h"
#include "sk64/skelatool_defs.h"
#include "shadow_map.h"
#include "../physics/point_constraint.h"
#include "../physics/debug_renderer.h"
#include "../controls/controller.h"
#include "../controls/controller_actions.h"
#include "../physics/collision_scene.h"
#include "../levels/static_render.h"
#include "../levels/levels.h"
#include "../savefile/checkpoint.h"
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
#include "render_plan.h"
#include "../menu/game_menu.h"

extern struct GameMenu gGameMenu;

struct LandingMenuOption gPauseMenuOptions[] = {
    {"RESUME", GameMenuStateResumeGame},
    {"SAVE GAME", GameMenuStateSaveGame},
    {"LOAD GAME", GameMenuStateLoadGame},
    {"NEW GAME", GameMenuStateNewGame},
    {"OPTIONS", GameMenuStateOptions},
    {"QUIT", GameMenuStateQuit},
};

Lights1 gSceneLights = gdSPDefLights1(128, 128, 128, 128, 128, 128, 0, 127, 0);

#define LEVEL_INDEX_WITH_GUN_0  2
#define LEVEL_INDEX_WITH_GUN_1  8

#define FADE_IN_

void sceneUpdateListeners(struct Scene* scene);

void sceneInitDynamicColliders(struct Scene* scene) {
    int boxCount = gCurrentLevel->dynamicBoxCount;

    struct CollisionObject* colliders = malloc(sizeof(struct CollisionObject) * boxCount);
    struct ColliderTypeData* colliderType = malloc(sizeof(struct ColliderTypeData) * boxCount);
    struct RigidBody* body = malloc(sizeof(struct RigidBody) * boxCount);

    for (int i = 0; i < boxCount; ++i) {
        colliderType[i].type = CollisionShapeTypeBox;
        colliderType[i].data = &gCurrentLevel->dynamicBoxes[i].box;
        colliderType[i].bounce = 0.5f;
        colliderType[i].friction = 0.5f;
        colliderType[i].callbacks = &gCollisionBoxCallbacks;
        collisionObjectInit(&colliders[i], &colliderType[i], &body[i], 1.0f, COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_BLOCK_BALL | COLLISION_LAYERS_STATIC);
        rigidBodyMarkKinematic(&body[i]);

        body[i].currentRoom = gCurrentLevel->dynamicBoxes[i].roomIndex;

        collisionSceneAddDynamicObject(&colliders[i]);
    }

    scene->dynamicColliders = colliders;
}

void sceneInit(struct Scene* scene) {
    sceneInitNoPauseMenu(scene);
    
    gameMenuInit(&gGameMenu, gPauseMenuOptions, sizeof(gPauseMenuOptions) / sizeof(*gPauseMenuOptions), 1);

    if (!checkpointExists()) {
        scene->checkpointState = SceneCheckpointStatePendingRender;
    } else {
        scene->checkpointState = SceneCheckpointStateSaved;
        checkpointLoadLast(scene);
    }

    if (gCurrentLevelIndex > gSaveData.header.chapterProgress) {
        gSaveData.header.chapterProgress = gCurrentLevelIndex;
        savefileSave();
    }

    gGameMenu.state = GameMenuStateResumeGame;
}

void sceneInitNoPauseMenu(struct Scene* scene) {
    signalsInit(1);

    cameraInit(&scene->camera, 70.0f, DEFAULT_NEAR_PLANE * SCENE_SCALE, DEFAULT_FAR_PLANE * SCENE_SCALE);

    struct Location* startLocation = levelGetLocation(gCurrentLevel->startLocation);
    struct Location combinedLocation;
    struct Vector3 startVelocity;
    combinedLocation.roomIndex = startLocation->roomIndex;
    transformConcat(&startLocation->transform, levelRelativeTransform(), &combinedLocation.transform);
    quatMultVector(&startLocation->transform.rotation, levelRelativeVelocity(), &startVelocity);

    portalGunInit(&scene->portalGun, &combinedLocation.transform);

    playerInit(&scene->player, &combinedLocation, &startVelocity, &scene->portalGun.collisionObject);

    scene->camera.transform.rotation = scene->player.lookTransform.rotation;
    scene->camera.transform.position = scene->player.lookTransform.position;

    sceneUpdateListeners(scene);

    if (gCurrentLevelIndex >= LEVEL_INDEX_WITH_GUN_0) {
        playerGivePortalGun(&scene->player, PlayerHasFirstPortalGun);
    }

    if (gCurrentLevelIndex >= LEVEL_INDEX_WITH_GUN_1) {
        playerGivePortalGun(&scene->player, PlayerHasSecondPortalGun);
    }

    portalInit(&scene->portals[0], 0);
    portalInit(&scene->portals[1], PortalFlagsOddParity);

    scene->buttonCount = gCurrentLevel->buttonCount;
    scene->buttons = malloc(sizeof(struct Button) * scene->buttonCount);

    for (int i = 0; i < scene->buttonCount; ++i) {
        buttonInit(&scene->buttons[i], &gCurrentLevel->buttons[i]);
    }

    if (checkpointExists()) {
        // if a checkpoint exists it will load the decor
        scene->decorCount = 0;
        scene->decor = NULL;
    } else {
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

    scene->switchCount = gCurrentLevel->switchCount;
    scene->switches = malloc(sizeof(struct Switch) * scene->switchCount);
    for (int i = 0; i < scene->switchCount; ++i) {
        switchInit(&scene->switches[i], &gCurrentLevel->switches[i]);
    }

    ballBurnMarkInit();

    scene->ballLancherCount = gCurrentLevel->ballLauncherCount;
    scene->ballLaunchers = malloc(sizeof(struct BallLauncher) * scene->ballLancherCount);
    for (int i = 0; i < scene->ballLancherCount; ++i) {
        ballLauncherInit(&scene->ballLaunchers[i], &gCurrentLevel->ballLaunchers[i]);
    }

    scene->ballCatcherCount = gCurrentLevel->ballCatcherCount;
    scene->ballCatchers = malloc(sizeof(struct BallCatcher) * scene->ballCatcherCount);
    for (int i = 0; i < scene->ballCatcherCount; ++i) {
        ballCatcherInit(&scene->ballCatchers[i], &gCurrentLevel->ballCatchers[i]);
    }

    scene->clockCount = gCurrentLevel->clockCount;
    scene->clocks = malloc(sizeof(struct Clock) * scene->clockCount);
    for (int i = 0; i < scene->clockCount; ++i) {
        clockInit(&scene->clocks[i], &gCurrentLevel->clocks[i]);
    }

    scene->securityCameraCount = gCurrentLevel->securityCameraCount;
    scene->securityCameras = malloc(sizeof(struct SecurityCamera) * scene->securityCameraCount);
    for (int i = 0 ; i < scene->securityCameraCount; ++i) {
        securityCameraInit(&scene->securityCameras[i], &gCurrentLevel->securityCameras[i]);
    }

    scene->last_portal_indx_shot=-1;
    scene->looked_wall_portalable_0=0;
    scene->looked_wall_portalable_1=0;
    scene->continuouslyAttemptingPortalOpen=0;
    scene->checkpointState = SceneCheckpointStateSaved;

    scene->freeCameraOffset = gZeroVec;

    sceneInitDynamicColliders(scene);

    sceneAnimatorInit(&scene->animator, gCurrentLevel->animations, gCurrentLevel->animationInfoCount);

    if (gCurrentLevelIndex == 0) {
        scene->fadeInTimer = INTRO_TOTAL_TIME;
    } else {
        scene->fadeInTimer = 0.0f;
    }
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

LookAt gLookAt = gdSPDefLookAt(127, 0, 0, 0, 127, 0);

void sceneRender(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task) {
    playerApplyCameraTransform(&scene->player, &scene->camera.transform);

    gSPSetLights1(renderState->dl++, gSceneLights);
    LookAt* lookAt = renderStateRequestLookAt(renderState);

    if (!lookAt) {
        return;
    }
    
    *lookAt = gLookAt;
    gSPLookAt(renderState->dl++, lookAt);

    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);

    struct RenderPlan renderPlan;

    Mtx* staticMatrices = sceneAnimatorBuildTransforms(&scene->animator, renderState);

    renderPlanBuild(&renderPlan, scene, renderState);
    renderPlanExecute(&renderPlan, scene, staticMatrices, renderState);

    portalGunRenderReal(&scene->portalGun, renderState);

    gDPPipeSync(renderState->dl++);
    gDPSetRenderMode(renderState->dl++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gSPGeometryMode(renderState->dl++, G_ZBUFFER | G_LIGHTING | G_CULL_BOTH, G_SHADE);

    hudRender(renderState, &scene->player, scene->last_portal_indx_shot, scene->looked_wall_portalable_0, scene->looked_wall_portalable_1, scene->fadeInTimer);

    if (gGameMenu.state != GameMenuStateResumeGame) {
        gameMenuRender(&gGameMenu, renderState, task);
    }

    // sceneRenderPerformanceMetrics(scene, renderState, task);

    // contactSolverDebugDraw(&gContactSolver, renderState);
}



void sceneCheckPortals(struct Scene* scene) {
    if (playerIsDead(&scene->player)) {
        sceneClosePortal(scene, 0);
        sceneClosePortal(scene, 1);
        portalCheckForHoles(scene->portals);
        return;
    }

    struct Ray raycastRay;
    struct Vector3 playerUp;
    raycastRay.origin = scene->player.lookTransform.position;
    vector3Negate(&gForward, &raycastRay.dir);
    quatMultVector(&scene->player.lookTransform.rotation, &raycastRay.dir, &raycastRay.dir);
    quatMultVector(&scene->player.lookTransform.rotation, &gUp, &playerUp);

    int fireBlue = controllerActionGet(ControllerActionOpenPortal0);
    int fireOrange = controllerActionGet(ControllerActionOpenPortal1);

    int hasBlue = (scene->player.flags & PlayerHasFirstPortalGun) != 0;
    int hasOrange = (scene->player.flags & PlayerHasSecondPortalGun) != 0;
    if (scene->continuouslyAttemptingPortalOpen){
        sceneFirePortal(scene, &scene->savedPortal.ray, &scene->savedPortal.transformUp, scene->savedPortal.portalIndex, scene->savedPortal.roomIndex, 0, 0);
    }

    if (fireOrange && hasOrange && !playerIsGrabbing(&scene->player)) {
        sceneFirePortal(scene, &raycastRay, &playerUp, 0, scene->player.body.currentRoom, 1, 0);
        scene->player.flags |= PlayerJustShotPortalGun;
        scene->last_portal_indx_shot=0;
        soundPlayerPlay(soundsPortalgunShoot[0], 1.0f, 1.0f, NULL, NULL);
    }

    if ((fireBlue || (!hasOrange && fireOrange)) && hasBlue && !playerIsGrabbing(&scene->player)) {
        sceneFirePortal(scene, &raycastRay, &playerUp, 1, scene->player.body.currentRoom, 1, 0);
        scene->player.flags |= PlayerJustShotPortalGun;
        scene->last_portal_indx_shot=1;
        soundPlayerPlay(soundsPortalgunShoot[1], 1.0f, 1.0f, NULL, NULL);
    }

    if ((fireOrange || fireBlue) && playerIsGrabbing(&scene->player)){
        playerSetGrabbing(&scene->player, NULL);
    }

    scene->looked_wall_portalable_0 = 0;
    scene->looked_wall_portalable_1 = 0;
    
    if ((scene->player.flags & PlayerFlagsGrounded) && (scene->player.flags & PlayerIsStepping)){
        soundPlayerPlay(soundsConcreteFootstep[scene->player.currentFoot], 1.0f, 1.0f, NULL, NULL);
        scene->player.flags &= ~PlayerIsStepping;
    }
    if (scene->player.flags & PlayerJustJumped){
        soundPlayerPlay(soundsConcreteFootstep[3], 1.0f, 1.0f, NULL, NULL);
        scene->player.flags &= ~PlayerJustJumped;
    }
    if (scene->player.flags & PlayerJustLanded){
        soundPlayerPlay(soundsConcreteFootstep[2], 1.0f, 1.0f, NULL, NULL);
        scene->player.flags &= ~PlayerJustLanded;
    }
    if (scene->player.flags & PlayerJustSelect){
        soundPlayerPlay(soundsSelecting[1], 1.0f, 0.5f, NULL, NULL);
        scene->player.flags &= ~PlayerJustSelect;
    }
    if (scene->player.flags & PlayerJustDeniedSelect){
        if (scene->player.flags & PlayerHasFirstPortalGun){
            soundPlayerPlay(soundsSelecting[0], 1.0f, 0.5f, NULL, NULL);
        }
        else{
            soundPlayerPlay(soundsSelecting[2], 1.0f, 0.5f, NULL, NULL);
        }
        scene->player.flags &= ~PlayerJustDeniedSelect;
    }

    if (scene->player.flags & PlayerHasFirstPortalGun){
        if (sceneFirePortal(scene, &raycastRay, &playerUp, 0, scene->player.body.currentRoom, 1, 1)){
            scene->looked_wall_portalable_0 = 1;
        }
        if (sceneFirePortal(scene, &raycastRay, &playerUp, 1, scene->player.body.currentRoom, 1, 1)){
            scene->looked_wall_portalable_1 = 1;
        }
    }

    if (scene->player.body.flags & RigidBodyFizzled) {
        if (scene->portals[0].flags & PortalFlagsPlayerPortal) {
            sceneClosePortal(scene, 0);
        }
        if (scene->portals[1].flags & PortalFlagsPlayerPortal) {
            sceneClosePortal(scene, 1);
        }
        scene->player.body.flags &= ~RigidBodyFizzled;
    }

    int isOpen = collisionSceneIsPortalOpen();

    portalUpdate(&scene->portals[0], isOpen);
    portalUpdate(&scene->portals[1], isOpen);

    portalCheckForHoles(scene->portals);
}

#define MAX_LISTEN_THROUGH_PORTAL_DISTANCE 3.0f

int sceneUpdatePortalListener(struct Scene* scene, int portalIndex, int listenerIndex) {
    if (vector3DistSqrd(&scene->player.lookTransform.position, &scene->portals[portalIndex].rigidBody.transform.position) > MAX_LISTEN_THROUGH_PORTAL_DISTANCE * MAX_LISTEN_THROUGH_PORTAL_DISTANCE) {
        return 0;
    }

    struct Transform otherInverse;
    transformInvert(&scene->portals[1 - portalIndex].rigidBody.transform, &otherInverse);
    struct Transform portalCombined;
    transformConcat(&scene->portals[portalIndex].rigidBody.transform, &otherInverse, &portalCombined);

    struct Transform relativeTransform;
    transformConcat(&portalCombined, &scene->player.lookTransform, &relativeTransform);

    struct Vector3 velocity;
    quatMultVector(&relativeTransform.rotation, &scene->player.body.velocity, &velocity);

    soundListenerUpdate(&relativeTransform.position, &relativeTransform.rotation, &velocity, listenerIndex);

    return 1;
}

void sceneUpdateListeners(struct Scene* scene) {
    soundListenerUpdate(&scene->player.lookTransform.position, &scene->player.lookTransform.rotation, &scene->player.body.velocity, 0);

    int listenerCount = 1;

    if (collisionSceneIsPortalOpen()) {
        if (sceneUpdatePortalListener(scene, 0, listenerCount)) {
            ++listenerCount;
        }

        if (sceneUpdatePortalListener(scene, 1, listenerCount)) {
            ++listenerCount;
        }
    }

    soundListenerSetCount(listenerCount);
}

struct Transform gRelativeElevatorTransform = {
    {0.0f, 0.0f, 0.0f},
    {0.5f, -0.5f, -0.5f, 0.5f},
    {1.0f, 1.0f, 1.0f},
};

void sceneUpdatePortalVelocity(struct Scene* scene) {
    for (int i = 0; i < 2; ++i) {
        struct Portal* portal = &scene->portals[i];

        if (portal->transformIndex == NO_TRANSFORM_INDEX) {
            continue;
        }

        struct Vector3 newPos;
        struct Transform baseTransform;

        sceneAnimatorTransformForIndex(&scene->animator, portal->transformIndex, &baseTransform);

        transformPoint(&baseTransform, &portal->relativePos, &newPos);

        // calculate new portal velocity
        struct Vector3 offset;
        vector3Sub(&newPos, &gCollisionScene.portalTransforms[i]->position, &offset);
        vector3Scale(&offset, &gCollisionScene.portalVelocity[i], 1.0 / FIXED_DELTA_TIME);

        // update portal position
        gCollisionScene.portalTransforms[i]->position = newPos;
        collisionObjectUpdateBB(&portal->collisionObject);
    }
}

#define FREE_CAM_VELOCITY   2.0f

void sceneUpdateAnimatedObjects(struct Scene* scene) {
    for (int i = 0; i < gCurrentLevel->dynamicBoxCount; ++i) {
        struct DynamicBoxDefinition* boxDef = &gCurrentLevel->dynamicBoxes[i];

        struct Transform baseTransform;

        sceneAnimatorTransformForIndex(&scene->animator, boxDef->transformIndex, &baseTransform);  

        struct Transform relativeTransform;
        relativeTransform.position = boxDef->position;
        relativeTransform.rotation = boxDef->rotation;   
        relativeTransform.scale = gOneVec;

        transformConcat(&baseTransform, &relativeTransform, &scene->dynamicColliders[i].body->transform);

        collisionObjectUpdateBB(&scene->dynamicColliders[i]);
    }
}

void sceneUpdate(struct Scene* scene) {
    if (scene->checkpointState == SceneCheckpointStateReady) {
        checkpointSave(scene);
        scene->checkpointState = SceneCheckpointStateSaved;
    }
    
    OSTime frameStart = osGetTime();
    scene->lastFrameTime = frameStart - scene->lastFrameStart;

    if (gGameMenu.state != GameMenuStateResumeGame) {
        if (controllerActionGet(ControllerActionPause) || 
            (gGameMenu.state == GameMenuStateLanding && controllerGetButtonDown(0, B_BUTTON))) {
            gGameMenu.state = GameMenuStateResumeGame;
            savefileSave();
        }

        gameMenuUpdate(&gGameMenu);

        if (gGameMenu.state == GameMenuStateResumeGame) {
            soundPlayerResume();
        }

        if (gGameMenu.state == GameMenuStateQuit) {
            levelQueueLoad(MAIN_MENU, NULL, NULL);
            return;
        }

        return;
    } else if (controllerActionGet(ControllerActionPause)) {
        savefileGrabScreenshot();
        gGameMenu.state = GameMenuStateLanding;
        gGameMenu.landingMenu.selectedItem = 0;
        soundPlayerPause();
    }

    signalsReset();

    if (sceneAnimatorIsRunning(&scene->animator, gCurrentLevel->playerAnimatorIndex)) {
        scene->player.flags |= PlayerInCutscene;
        sceneAnimatorTransformForIndex(&scene->animator, gCurrentLevel->playerAnimatorIndex, &scene->player.lookTransform);
        scene->player.body.transform = scene->player.lookTransform;
    } else if (scene->player.flags & PlayerInCutscene) {
        scene->player.flags &= ~PlayerInCutscene;
    }

    // objects that can fizzle need to update before the player
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

    for (int i = 0; i < scene->securityCameraCount; ++i) {
        securityCameraUpdate(&scene->securityCameras[i]);
    }

    portalGunUpdate(&scene->portalGun, &scene->player);
    playerUpdate(&scene->player);
    sceneUpdateListeners(scene);
    sceneCheckPortals(scene);

    if ((playerIsDead(&scene->player) && (controllerActionGet(ControllerActionPause) || controllerActionGet(ControllerActionJump))) ||
        scene->player.lookTransform.position.y < KILL_PLANE_Y) {
        levelLoadLastCheckpoint();
    }
    
    for (int i = 0; i < scene->buttonCount; ++i) {
        buttonUpdate(&scene->buttons[i]);
    }

    for (int i = 0; i < scene->switchCount; ++i) {
        switchUpdate(&scene->switches[i]);
    }

    for (int i = 0; i < scene->ballCatcherCount; ++i) {
        ballCatcherUpdate(&scene->ballCatchers[i], scene->ballLaunchers, scene->ballLancherCount);
    }

    cutsceneCheckTriggers(&scene->player.lookTransform.position);
    signalsEvaluateSignals(gCurrentLevel->signalOperators, gCurrentLevel->signalOperatorCount);

    for (int i = 0; i < scene->doorCount; ++i) {
        doorUpdate(&scene->doors[i]);
    }

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
                    &gZeroVec,
                    &gZeroVec,
                    scene->elevators[teleportTo].roomIndex
                );
                rigidBodyTeleport(
                    &scene->portalGun.rigidBody,
                    &scene->elevators[i].rigidBody.transform,
                    &scene->elevators[teleportTo].rigidBody.transform,
                    &gZeroVec,
                    &gZeroVec,
                    scene->elevators[teleportTo].roomIndex
                );
                sceneQueueCheckpoint(&gScene);
                sceneClosePortal(&gScene, 0);
                sceneClosePortal(&gScene, 1);
            }
        }
    }

    for (int i = 0; i < scene->pedestalCount; ++i) {
        pedestalUpdate(&scene->pedestals[i]);
    }

    for (int i = 0; i < scene->boxDropperCount; ++i) {
        boxDropperUpdate(&scene->boxDroppers[i]);
    }

    for (int i = 0; i < scene->ballLancherCount; ++i) {
        ballLauncherUpdate(&scene->ballLaunchers[i]);
    }

    sceneAnimatorUpdate(&scene->animator);
    sceneUpdatePortalVelocity(scene);
    sceneUpdateAnimatedObjects(scene);
    
    collisionSceneUpdateDynamics();

    cutscenesUpdate();

    scene->cpuTime = osGetTime() - frameStart;
    scene->lastFrameStart = frameStart;

    OSContPad* freecam = controllersGetControllerData(2);


    struct Vector3 lookDir;
    struct Vector3 rightDir;

    playerGetMoveBasis(&scene->camera.transform, &lookDir, &rightDir);

    if (freecam->stick_y) {
        if (controllerGetButton(2, Z_TRIG)) {
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

    if (controllerGetButtonDown(2, START_BUTTON)) {
        scene->freeCameraOffset = gZeroVec;
    }

    vector3Add(&scene->camera.transform.position, &scene->freeCameraOffset, &scene->camera.transform.position);

    if (controllerGetButtonDown(2, L_TRIG)) {
        levelQueueLoad(NEXT_LEVEL, NULL, NULL);
    }

    if (scene->fadeInTimer > 0.0f) {
        scene->fadeInTimer -= FIXED_DELTA_TIME;

        if (scene->fadeInTimer < 0.0f) {
            scene->fadeInTimer = 0.0f;
        }
    }
}

void sceneQueueCheckpoint(struct Scene* scene) {
    scene->checkpointState = SceneCheckpointStateReady;
}


void sceneCheckSecurityCamera(struct Scene* scene, struct Portal* portal) {
    struct Box3D portalBB;
    portalCalculateBB(portal, &portalBB);
    securityCamerasCheckPortal(scene->securityCameras, scene->securityCameraCount, &portalBB);
}

int sceneOpenPortal(struct Scene* scene, struct Transform* at, int transformIndex, int portalIndex, struct PortalSurfaceMappingRange surfaceMapping, struct CollisionObject* collisionObject, int roomIndex, int fromPlayer, int just_checking) {
    struct Transform finalAt;

    struct Transform relativeToTransform;
    
    if (transformIndex != NO_TRANSFORM_INDEX) {
        sceneAnimatorTransformForIndex(&scene->animator, transformIndex, &relativeToTransform);
        struct Transform relativeInverse;
        transformInvert(&relativeToTransform, &relativeInverse);
        transformConcat(&relativeInverse, at, &finalAt);
    } else {
        finalAt = *at;
    }

    int wasOpen = collisionSceneIsPortalOpen();

    for (int indexIndex = surfaceMapping.minPortalIndex; indexIndex < surfaceMapping.maxPortalIndex; ++indexIndex) {
        int surfaceIndex = gCurrentLevel->portalSurfaceMappingIndices[indexIndex];

        struct PortalSurface* existingSurface = portalSurfaceGetOriginalSurface(surfaceIndex, portalIndex);

        struct Portal* portal = &scene->portals[portalIndex];

        if (portalAttachToSurface(portal, existingSurface, surfaceIndex, &finalAt, just_checking)) {
            if (just_checking){
                return 1;
            }

            collisionScenePushObjectsOutOfPortal(portalIndex);

            // the portal position may have been adjusted
            if (transformIndex != NO_TRANSFORM_INDEX) {
                portal->relativePos = finalAt.position;
                transformConcat(&relativeToTransform, &finalAt, &portal->rigidBody.transform);
            } else {
                portal->rigidBody.transform = finalAt;
            }
            
            gCollisionScene.portalVelocity[portalIndex] = gZeroVec;
            portal->transformIndex = transformIndex;
            portal->rigidBody.currentRoom = roomIndex;
            portal->colliderIndex = levelQuadIndex(collisionObject);
            portal->scale = 0.0f;
            collisionSceneSetPortal(portalIndex, &portal->rigidBody.transform, roomIndex, portal->colliderIndex);
            collisionObjectUpdateBB(&portal->collisionObject);

            soundPlayerPlay(soundsPortalOpen2, 1.0f, 1.0f, &portal->rigidBody.transform.position, &gZeroVec);

            if (fromPlayer) {
                portal->flags |= PortalFlagsPlayerPortal;
            } else {
                portal->flags &= ~PortalFlagsPlayerPortal;
            }

            if (collisionSceneIsPortalOpen()) {
                // the second portal is fully transparent right away
                portal->opacity = 0.0f;

                if (!fromPlayer) {
                    // flash other portal to make it easier to tell
                    // something changed and play sound near other portal
                    struct Portal* otherPortal = &scene->portals[1 - portalIndex];
                    otherPortal->opacity = 1.0f;
                    soundPlayerPlay(soundsPortalOpen2, 1.0f, 1.0f, &otherPortal->rigidBody.transform.position, &gZeroVec);
                }

                sceneCheckSecurityCamera(scene, portal);

                if (!wasOpen) {
                    sceneCheckSecurityCamera(scene, &scene->portals[1 - portalIndex]);
                }
            }

            contactSolverCheckPortalContacts(&gContactSolver, collisionObject);
            ballBurnFilterOnPortal(&portal->rigidBody.transform, portalIndex);
            playerSignalPortalChanged(&scene->player);
            return 1;
        }
    }

    return 0;
}

int sceneDynamicBoxIndex(struct Scene* scene, struct CollisionObject* hitObject) {
    if (hitObject < scene->dynamicColliders || hitObject >= scene->dynamicColliders + gCurrentLevel->dynamicBoxCount) {
        return -1;
    }

    return hitObject - scene->dynamicColliders;
}

int sceneDetermineSurfaceMapping(struct Scene* scene, struct CollisionObject* hitObject, struct PortalSurfaceMappingRange* mappingRangeOut, int* relativeToOut) {
    int quadIndex = levelQuadIndex(hitObject);

    if (quadIndex != -1) {
        *mappingRangeOut = gCurrentLevel->portalSurfaceMappingRange[quadIndex];
        *relativeToOut = NO_TRANSFORM_INDEX;
        return 1;
    } 

    int dynamicBoxIndex = sceneDynamicBoxIndex(scene, hitObject);

    if (dynamicBoxIndex != -1) {
        *mappingRangeOut = gCurrentLevel->portalSurfaceDynamicMappingRange[dynamicBoxIndex];
        *relativeToOut = gCurrentLevel->dynamicBoxes[dynamicBoxIndex].transformIndex;
        return 1;
    }

    return 0;
}

int sceneFirePortal(struct Scene* scene, struct Ray* ray, struct Vector3* playerUp, int portalIndex, int roomIndex, int fromPlayer, int just_checking) {
    struct RaycastHit hit;

    if (!collisionSceneRaycast(&gCollisionScene, roomIndex, ray, COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_PORTAL, 1000000.0f, 0, &hit)) {
        return 0;
    }

    struct PortalSurfaceMappingRange mappingRange;
    int relativeIndex = NO_TRANSFORM_INDEX;

    if (!sceneDetermineSurfaceMapping(scene, hit.object, &mappingRange, &relativeIndex)) {
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

    if (!sceneOpenPortal(scene, &portalLocation, relativeIndex, portalIndex, mappingRange, hit.object, hit.roomIndex, fromPlayer, just_checking) && !fromPlayer){
        sceneClosePortal(scene, 1-portalIndex);
        scene->continuouslyAttemptingPortalOpen = 1;
        scene->savedPortal.portalIndex = portalIndex;
        scene->savedPortal.ray = *ray;
        scene->savedPortal.roomIndex = roomIndex;
        scene->savedPortal.transformUp = *playerUp;
        return 0;
    }
    if (!fromPlayer){
        scene->continuouslyAttemptingPortalOpen = 0;
    }
    return 1;
}

void sceneClosePortal(struct Scene* scene, int portalIndex) {
    if (scene->player.body.flags & (RigidBodyIsTouchingPortalA|RigidBodyIsTouchingPortalB|RigidBodyWasTouchingPortalA|RigidBodyWasTouchingPortalB|RigidBodyFlagsCrossedPortal0|RigidBodyFlagsCrossedPortal1)){
        return;
    } 
    else if (gCollisionScene.portalTransforms[portalIndex]) {
        soundPlayerPlay(soundsPortalFizzle, 1.0f, 1.0f, &gCollisionScene.portalTransforms[portalIndex]->position, &gZeroVec);
        gCollisionScene.portalTransforms[portalIndex] = NULL;
        gCollisionScene.portalColliderIndex[portalIndex] = -1;
        scene->portals[portalIndex].flags |= PortalFlagsNeedsNewHole;
        scene->portals[portalIndex].portalSurfaceIndex = -1;
        scene->portals[portalIndex].transformIndex = NO_TRANSFORM_INDEX;

        collisionSceneRemoveDynamicObject(&scene->portals[portalIndex].collisionObject);
    }
    return;
}
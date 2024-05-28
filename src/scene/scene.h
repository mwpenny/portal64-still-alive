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
#include "hud.h"
#include "pedestal.h"
#include "signage.h"
#include "box_dropper.h"
#include "scene_animator.h"
#include "switch.h"
#include "ball_launcher.h"
#include "ball_catcher.h"
#include "portal_gun.h"
#include "clock.h"
#include "security_camera.h"
#include "trigger_listener.h"
#include "../effects/effects.h"

struct SavedPortal {
    struct Ray ray;
    struct Vector3 transformUp;
    int portalIndex;
    int roomIndex;
};

enum SceneCheckpointState {
    SceneCheckpointStateSaved,
    SceneCheckpointStatePendingRender,
    SceneCheckpointStateReady,
};

struct Scene {
    struct Camera camera;
    struct Player player;
    struct PortalGun portalGun;
    struct Portal portals[2];
    struct Button* buttons;
    struct DecorObject** decor;
    struct TriggerListener* triggerListeners;
    struct Door* doors;
    struct Fizzler* fizzlers;
    struct Elevator* elevators;
    struct Pedestal* pedestals;
    struct Signage* signage;
    struct BoxDropper* boxDroppers;
    struct Switch* switches;
    struct Vector3 freeCameraOffset;
    struct SceneAnimator animator;
    struct CollisionObject* dynamicColliders;
    struct BallLauncher* ballLaunchers;
    struct BallCatcher* ballCatchers;
    struct Clock* clocks;
    struct SecurityCamera* securityCameras;
    struct SavedPortal savedPortal;
    struct Effects effects;
    struct Hud hud;
    Time cpuTime;
    Time lastFrameStart;
    Time lastFrameTime;
    u8 buttonCount;
    u8 decorCount;
    u8 triggerListenerCount;
    u8 doorCount;
    u8 fizzlerCount;
    u8 elevatorCount;
    u8 pedestalCount;
    u8 signageCount;
    u8 boxDropperCount;
    u8 switchCount;
    u8 ballLancherCount;
    u8 ballCatcherCount;
    u8 clockCount;
    u8 securityCameraCount;
    u8 boolCutsceneIsRunning;

    u8 continuouslyAttemptingPortalOpen;
    u8 checkpointState;
    u8 mainMenuMode;
};

extern struct Scene gScene;

struct GraphicsTask;

void sceneInit(struct Scene* scene);
void sceneInitNoPauseMenu(struct Scene* scene, int mainMenuMode);
void sceneRender(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task);
void sceneUpdate(struct Scene* scene);
void sceneQueueCheckpoint(struct Scene* scene);

int sceneOpenPortalFromHit(struct Scene* scene, struct Ray* ray, struct RaycastHit* hit, struct Vector3* playerUp, int portalIndex, int roomIndex, int fromPlayer, int just_checking);
int sceneFirePortal(struct Scene* scene, struct Ray* ray, struct Vector3* playerUp, int portalIndex, int roomIndex, int fromPlayer, int just_checking);
int sceneClosePortal(struct Scene* scene, int portalIndex);

#endif

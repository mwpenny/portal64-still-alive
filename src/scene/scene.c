
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
#include "../controls/controller.h"

#include "../levels/test_chamber_00_0/header.h"

struct Vector3 gStartPosition = {5.0f, 0.0f, -5.0f};

void sceneInit(struct Scene* scene) {
    cameraInit(&scene->camera, 45.0f, 0.25f * SCENE_SCALE, 80.0f * SCENE_SCALE);
    playerInit(&scene->player);

    scene->player.transform.position = gStartPosition;
    quatAxisAngle(&gUp, M_PI, &scene->player.transform.rotation);

    portalInit(&scene->portals[0], 0);
    portalInit(&scene->portals[1], PortalFlagsOddParity);

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

    portalRender(&scene->portals[0], &scene->portals[1], properties, sceneRenderWithProperties, data, renderState);
    portalRender(&scene->portals[1], &scene->portals[0], properties, sceneRenderWithProperties, data, renderState);

    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);
    gSPDisplayList(renderState->dl++, test_chamber_00_0_test_chamber_00_0_mesh);

    cubeRender(&scene->cube, renderState);
}

void sceneRender(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task) {
    struct RenderProps renderProperties;

    renderPropsInit(&renderProperties, &scene->camera, (float)SCREEN_WD / (float)SCREEN_HT, renderState);

    renderProperties.camera = scene->camera;
    renderProperties.currentDepth = STARTING_RENDER_DEPTH;

    sceneRenderWithProperties(scene, &renderProperties, renderState);
}

unsigned ignoreInputFrames = 10;

void sceneUpdate(struct Scene* scene) {
    playerUpdate(&scene->player, &scene->camera.transform);
    cubeUpdate(&scene->cube);

    if (controllerGetButtonDown(0, B_BUTTON)) {
        if (scene->player.grabbing) {
            scene->player.grabbing = NULL;
        } else {
            scene->player.grabbing = &scene->cube.rigidBody;
        }
    }
}
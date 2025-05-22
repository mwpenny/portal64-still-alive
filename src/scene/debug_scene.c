#include "debug_scene.h"

#include <ultra64.h>

#include "dynamic_scene.h"
#include "font/font.h"
#include "font/liberation_mono.h"
#include "levels/levels.h"
#include "physics/collision_scene.h"
#include "player/player.h"
#include "system/controller.h"
#include "system/time.h"

#include "codegen/assets/materials/ui.h"

#define FREE_CAM_VELOCITY        (2.0f / 80.0f)

#define PERF_METRICS_MARGIN      33
#define PERF_METRIC_ROW_PADDING  4
#define PERF_BAR_WIDTH           (SCREEN_WD - (PERF_METRICS_MARGIN * 2))
#define PERF_BAR_HEIGHT          6

static float lastFrameTimeMs     = 0.0f;
static float lastCpuTimeMs       = 0.0f;
static float lastUpdateTimeMs    = 0.0f;

static void debugSceneUpdateFreeCamera(struct Scene* scene) {
    ControllerStick freecam_stick = controllerGetStick(2);

    if (freecam_stick.x || freecam_stick.y) {
        struct Vector3 lookDir;
        struct Vector3 rightDir;
        playerGetMoveBasis(&scene->camera.transform.rotation, &lookDir, &rightDir);

        if (freecam_stick.y) {
            if (controllerGetButton(2, BUTTON_Z)) {
                vector3AddScaled(
                    &scene->freeCameraOffset,
                    &lookDir,
                    -freecam_stick.y * FREE_CAM_VELOCITY * FIXED_DELTA_TIME,
                    &scene->freeCameraOffset
                );
            } else {
                scene->freeCameraOffset.y += freecam_stick.y * FREE_CAM_VELOCITY * FIXED_DELTA_TIME;
            }
        }

        if (freecam_stick.x) {
            vector3AddScaled(
                &scene->freeCameraOffset,
                &rightDir,
                freecam_stick.x * FREE_CAM_VELOCITY * FIXED_DELTA_TIME,
                &scene->freeCameraOffset
            );
        }
    }

    if (controllerGetButtonDown(2, BUTTON_START)) {
        scene->freeCameraOffset = gZeroVec;
    }
}

static void debugSceneRenderTextMetric(struct FontRenderer* renderer, char* text, int y, struct RenderState* renderState) {
    fontRendererLayout(renderer, &gLiberationMonoFont, text, SCREEN_WD);

    renderState->dl = fontRendererBuildGfx(
        renderer,
        gLiberationMonoImages,
        PERF_METRICS_MARGIN,
        y - renderer->height,
        &gColorWhite,
        renderState->dl
    );
}

static uint64_t debugSceneVisibleRooms(struct RenderPlan* renderPlan) {
    uint64_t visibleRooms = 0;

    // Account for rooms visible from player POV and through portals
    for (int i = 0; i < renderPlan->stageCount; ++i) {
        visibleRooms |= renderPlan->stageProps[i].visiblerooms;
    }

    return visibleRooms;
}

static int debugSceneVisibleRoomCount(uint64_t visibleRooms) {
    int roomCount = 0;
    while (visibleRooms) {
        ++roomCount;
        visibleRooms &= visibleRooms - 1;
    }

    return roomCount;
}

// Average time-based metrics over 2 frames for more stable output
static float debugSceneAveragedTimeMs(float value, float* prevValue) {
    float ms = timeMicroseconds(value) / 1000.0f;
    float averaged = (ms + *prevValue) / 2.0f;
    *prevValue = ms;

    return averaged;
}

static void debugSceneRenderPerformanceMetrics(struct Scene* scene, struct RenderState* renderState, struct RenderPlan* renderPlan) {
    if (!gLastFrameTime) {
        return;
    }

    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gDPSetCycleType(renderState->dl++, G_CYC_1CYCLE);
    gDPSetFillColor(renderState->dl++, (GPACK_RGBA5551(0, 0, 0, 1) << 16 | GPACK_RGBA5551(0, 0, 0, 1)));
    gDPSetCombineLERP(
        renderState->dl++,
        0, 0, 0, ENVIRONMENT, 0, 0, 0, ENVIRONMENT,
        0, 0, 0, ENVIRONMENT, 0, 0, 0, ENVIRONMENT
    );
    gDPSetEnvColor(renderState->dl++, 32, 32, 32, 255);
    gSPTextureRectangle(renderState->dl++, 32 << 2, 32 << 2, (32 + 256) << 2, (32 + 8) << 2, 0, 0, 0, 1, 1);
    gSPTextureRectangle(renderState->dl++, 32 << 2, 44 << 2, (32 + 256) << 2, (44 + 8) << 2, 0, 0, 0, 1, 1);
    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, 32, 255, 32, 255);

    float cpuUsage = scene->cpuTime / (float)gLastFrameTime;
    gSPTextureRectangle(
        renderState->dl++,
        PERF_METRICS_MARGIN << 2,
        PERF_METRICS_MARGIN << 2,
        (int)(PERF_METRICS_MARGIN + (PERF_BAR_WIDTH * cpuUsage)) << 2,
        (PERF_METRICS_MARGIN + PERF_BAR_HEIGHT) << 2,
        0,
        0, 0,
        1, 1
    );

    float memoryUsage = renderStateMemoryUsage(renderState);
    gSPTextureRectangle(
        renderState->dl++,
        PERF_METRICS_MARGIN << 2,
        (PERF_METRICS_MARGIN + (PERF_BAR_HEIGHT * 2)) << 2,
        (int)(32 + 254 * memoryUsage) << 2,
        (PERF_METRICS_MARGIN + (PERF_BAR_HEIGHT * 3)) << 2,
        0,
        0, 0,
        1, 1
    );
    gDPPipeSync(renderState->dl++);

    struct FontRenderer fontRenderer;
    char metricText[16];
    int textY = SCREEN_HT - PERF_METRICS_MARGIN;

    float dt = debugSceneAveragedTimeMs(gLastFrameTime, &lastFrameTimeMs);

    uint64_t visibleRooms = debugSceneVisibleRooms(renderPlan);
    int roomCount = debugSceneVisibleRoomCount(visibleRooms);

    sprintf(metricText, "COL: %d/%d %d/%d",
        collisionSceneDynamicObjectCount(), MAX_DYNAMIC_COLLISION,
        contactSolverActiveManifoldCount(&gContactSolver), MAX_CONTACT_COUNT
    );
    debugSceneRenderTextMetric(&fontRenderer, metricText, textY, renderState);

    textY -= fontRenderer.height - PERF_METRIC_ROW_PADDING;
    sprintf(metricText, "VDO: %d/%d", dynamicSceneViewDependentObjectCount(), MAX_VIEW_DEPENDENT_OBJECTS);
    debugSceneRenderTextMetric(&fontRenderer, metricText, textY, renderState);

    textY -= fontRenderer.height - PERF_METRIC_ROW_PADDING;
    sprintf(metricText, "OBJ: %d/%d", dynamicSceneObjectCount(), MAX_DYNAMIC_SCENE_OBJECTS);
    debugSceneRenderTextMetric(&fontRenderer, metricText, textY, renderState);

    textY -= fontRenderer.height - PERF_METRIC_ROW_PADDING;
    sprintf(metricText, "RMS: %d %llx", roomCount, visibleRooms);
    debugSceneRenderTextMetric(&fontRenderer, metricText, textY, renderState);

    textY -= fontRenderer.height - PERF_METRIC_ROW_PADDING;
    sprintf(metricText, "UPD: %2.2f", debugSceneAveragedTimeMs(scene->updateTime, &lastUpdateTimeMs));
    debugSceneRenderTextMetric(&fontRenderer, metricText, textY, renderState);

    textY -= fontRenderer.height - PERF_METRIC_ROW_PADDING;
    sprintf(metricText, "CPU: %2.2f", debugSceneAveragedTimeMs(scene->cpuTime, &lastCpuTimeMs));
    debugSceneRenderTextMetric(&fontRenderer, metricText, textY, renderState);

    textY -= fontRenderer.height - PERF_METRIC_ROW_PADDING;
    sprintf(metricText, " DT: %2.2f", dt);
    debugSceneRenderTextMetric(&fontRenderer, metricText, textY, renderState);

    textY -= fontRenderer.height - PERF_METRIC_ROW_PADDING;
    sprintf(metricText, "FPS: %2.2f", 1000.0f / dt);
    debugSceneRenderTextMetric(&fontRenderer, metricText, textY, renderState);
}

void debugSceneInit(struct Scene* scene) {
    scene->freeCameraOffset = gZeroVec;
    scene->showPerformanceMetrics = 0;
    scene->showCollisionContacts = 0;
    scene->hideCurrentRoom = 0;
}

void debugSceneUpdate(struct Scene* scene) {
    if (!controllerIsConnected(2)) {
        return;
    }

    debugSceneUpdateFreeCamera(scene);

    if (controllerGetButtonDown(2, BUTTON_L)) {
        levelQueueLoad(NEXT_LEVEL, NULL, NULL);
    }

    if (controllerGetButtonDown(2, BUTTON_R)) {
        scene->hideCurrentRoom ^= 1;
    }

    if (controllerGetButtonDown(2, BUTTON_LEFT)) {
        scene->showPerformanceMetrics ^= 1;
    }

    if (controllerGetButtonDown(2, BUTTON_RIGHT)) {
        scene->showCollisionContacts ^= 1;
    }
}

void debugSceneRender(struct Scene* scene, struct RenderState* renderState, struct RenderPlan* renderPlan) {
    if (scene->showPerformanceMetrics) {
        debugSceneRenderPerformanceMetrics(scene, renderState, renderPlan);
    }
}

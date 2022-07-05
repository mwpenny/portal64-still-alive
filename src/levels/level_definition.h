#ifndef __LEVEL_DEFINITION_H__
#define __LEVEL_DEFINITION_H__

#include "../physics/world.h"
#include "../scene/portal_surface.h"
#include "../scene/signals.h"
#include "../math/boxs16.h"
#include "../math/box3d.h"
#include "../math/range.h"

struct StaticContentElement {
    Gfx* displayList;
    u8 materialIndex;
};

struct BoundingSphere {
    short x, y, z;
    short radius;
};

enum TriggerCutsceneIndex {
    TRIGGER_START,
};

enum CutsceneStepType {
    CutsceneStepTypeNoop,
    CutsceneStepTypePlaySound,
    CutsceneStepTypeStartSound,
    CutsceneStepTypeDelay,
    CutsceneStepTypeOpenPortal,
    CutsceneStepTypeSetSignal,
    CutsceneStepTypeWaitForSignal,
    CutsceneStepTypeTeleportPlayer,
    CutsceneStepTypeLoadLevel,
    CutsceneStepTypeGoto,
    CutsceneStepTypeStartCutscene,
    CutsceneStepTypeStopCutscene,
    CutsceneStepTypeWaitForCutscene,
};

struct CutsceneStep {
    enum CutsceneStepType type;

    union {
        struct {
            u16 soundId;
            u8 volume;
            u8 pitch;
        } playSound;
        struct {
            u16 locationIndex;
            u16 portalIndex;
        } openPortal;
        float delay;
        struct {
            u16 signalIndex;
            u16 signalValue;
        } setSignal;
        struct {
            u16 signalIndex;
        } waitForSignal;
        struct {
            u16 fromLocation;
            u16 toLocation;
        } teleportPlayer;
        struct {
            u16 fromLocation;
            u16 levelIndex;
        } loadLevel;
        struct {
            s16 relativeInstructionIndex;
        } gotoStep;
        struct {
            u16 cutsceneIndex;
        } cutscene;
        int noop;
    };
};

struct Cutscene {
    struct CutsceneStep* steps;
    u16 stepCount;
};

struct Trigger {
    struct Box3D box;
    short cutsceneIndex;
};

struct Location {
    struct Transform transform;
    short roomIndex;
};

struct DoorDefinition {
    struct Vector3 location;
    struct Quaternion rotation;
    short doorwayIndex;
    short signalIndex;
};

struct ButtonDefinition {
    struct Vector3 location;
    short roomIndex;
    short signalIndex;
    short cubeSignalIndex;
};

struct DecorDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
    short decorId;
};

struct FizzlerDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    float width;
    float height;
    short roomIndex;
};

struct ElevatorDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
    short signalIndex;
    short isExit;
};

struct LevelDefinition {
    struct CollisionObject* collisionQuads;
    struct StaticContentElement *staticContent;
    struct Rangeu16 *roomStaticMapping;
    struct BoundingBoxs16* staticBoundingBoxes;
    struct PortalSurface* portalSurfaces;
    // maps index of a collisionQuads to indices in portalSurfaces
    struct PortalSurfaceMappingRange* portalSurfaceMappingRange;
    u8* portalSurfaceMappingIndices;
    struct Trigger* triggers;
    struct Cutscene* cutscenes;
    struct Location* locations;
    struct World world;
    struct DoorDefinition* doors;
    struct ButtonDefinition* buttons;
    struct SignalOperator* signalOperators;
    struct DecorDefinition* decor;
    struct FizzlerDefinition* fizzlers;
    struct ElevatorDefinition* elevators;
    short collisionQuadCount;
    short staticContentCount;
    short portalSurfaceCount;
    short triggerCount;
    short cutsceneCount;
    short locationCount;
    short doorCount;
    short buttonCount;
    short signalOperatorCount;
    short decorCount;
    short fizzlerCount;
    short elevatorCount;
    short startLocation;
};

#endif
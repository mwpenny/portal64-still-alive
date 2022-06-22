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
    CutsceneWaitForElevator,
    CutsceneOpenElevator,
    CutsceneTeleportPlayer,
    CutsceneLoadLevel,
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
            u16 elevatorIndex;
        } waitForElevator;
        struct {
            u16 elevatorIndex;
        } openElevator;
        struct {
            u16 fromLocation;
            u16 toLocation;
        } teleportPlayer;
        struct {
            u16 fromLocation;
            u16 levelIndex;
        } loadLevel;
        int noop;
    };
};

struct Cutscene {
    struct CutsceneStep* steps;
    u16 stepCount;
};

struct Trigger {
    struct Cutscene cutscene;
    struct Box3D box;
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

struct LevelDefinition {
    struct CollisionObject* collisionQuads;
    struct StaticContentElement *staticContent;
    struct Rangeu16 *roomStaticMapping;
    struct BoundingBoxs16* staticBoundingBoxes;
    struct PortalSurface* portalSurfaces;
    // maps index of a collisionQuads to indices in portalSurfaces
    struct PortalSurfaceMapping* portalSurfaceMapping;
    struct Trigger* triggers;
    struct Location* locations;
    struct World world;
    struct DoorDefinition* doors;
    struct ButtonDefinition* buttons;
    struct SignalOperator* signalOperators;
    struct DecorDefinition* decor;
    struct FizzlerDefinition* fizzlers;
    short collisionQuadCount;
    short staticContentCount;
    short portalSurfaceCount;
    short triggerCount;
    short locationCount;
    short doorCount;
    short buttonCount;
    short signalOperatorCount;
    short decorCount;
    short fizzlerCount;
    short startLocation;
};

#endif
#ifndef __LEVEL_DEFINITION_H__
#define __LEVEL_DEFINITION_H__

#include "../physics/world.h"
#include "../scene/portal_surface.h"
#include "../scene/signals.h"
#include "../math/boxs16.h"
#include "../math/box3d.h"
#include "../math/range.h"
#include "../sk64/skelatool_clip.h"
#include "../sk64/skelatool_armature.h"
#include "../physics/collision_box.h"

#define NO_TRANSFORM_INDEX  0xFF

struct StaticContentElement {
    Gfx* displayList;
    u8 materialIndex;
    u8 transformIndex;
};

struct BoundingSphere {
    short x, y, z;
    short radius;
};

enum CutsceneStepType {
    CutsceneStepTypeNoop,
    CutsceneStepTypePlaySound,
    CutsceneStepTypeStartSound,
    CutsceneStepTypeQueueSound,
    CutsceneStepTypeWaitForChannel,
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
    CutsceneStepTypeHidePedestal,
    CutsceneStepTypePointPedestal,
    CutsceneStepPlayAnimation,
    CutsceneStepSetAnimationSpeed,
    CutsceneStepWaitForAnimation,
    CutsceneStepSaveCheckpoint,
    CutsceneStepKillPlayer,
    CutsceneStepTypeClosePortal,
};

#define CH_NONE    0xFF
#define CH_GLADOS  0

#define CH_COUNT   1

struct CutsceneStep {
    enum CutsceneStepType type;

    union {
        struct {
            u16 soundId;
            u8 volume;
            u8 pitch;
        } playSound;
        struct {
            u16 soundId;
            u8 channel;
            u8 volume;
        } queueSound;
        struct {
            u8 channel;
        } waitForChannel;
        struct {
            u16 locationIndex;
            u16 portalIndex;
        } openPortal;
        struct {
            u16 portalIndex;
        } closePortal;
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
            // -1 signals loading the next level
            s16 levelIndex;
        } loadLevel;
        struct {
            s16 relativeInstructionIndex;
        } gotoStep;
        struct {
            u16 cutsceneIndex;
        } cutscene;
        struct {
            u16 atLocation;
        } pointPedestal;
        struct {
            u8 armatureIndex;
            u8 animationIndex;
            s8 playbackSpeed;
        } playAnimation;
        struct {
            u8 armatureIndex;
            s8 playbackSpeed;
        } setAnimationSpeed;
        struct {
            u8 armatureIndex;
        } waitForAnimation;
        struct {
            u8 isWater;
        } killPlayer;
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
    short signalIndex;
};

struct Location {
    struct Transform transform;
    short roomIndex;
};

enum DoorType {
    DoorType01,
    DoorType02,
};

struct DoorDefinition {
    struct Vector3 location;
    struct Quaternion rotation;
    short doorwayIndex;
    short signalIndex;
    short doorType;
};

struct ButtonDefinition {
    struct Vector3 location;
    short roomIndex;
    short signalIndex;
    short cubeSignalIndex;
};

struct SwitchDefinition {
    struct Vector3 location;
    struct Quaternion rotation;
    short roomIndex;
    short signalIndex;
    float duration;
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
    short targetElevator;
};

struct PedestalDefinition {
    struct Vector3 position;
    short roomIndex;
};

struct SignageDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
    short testChamberNumber;
};

struct BoxDropperDefinition {
    struct Vector3 position;
    short roomIndex;
    short signalIndex;
};

struct AnimationInfo {
    struct SKArmatureDefinition armature;
    struct SKAnimationClip* clips;
    short clipCount;
};

struct DynamicBoxDefinition {
    struct CollisionBox box;
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
    short transformIndex;
};

struct BallLauncherDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
    short signalIndex;
    float ballLifetime;
};

struct BallCatcherDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
    short signalIndex;
};

struct LevelDefinition {
    struct CollisionObject* collisionQuads;
    struct StaticContentElement *staticContent;
    struct Rangeu16 *roomStaticMapping;
    struct BoundingBoxs16* staticBoundingBoxes;
    struct PortalSurface* portalSurfaces;
    // maps index of a collisionQuads to indices in portalSurfaces
    struct PortalSurfaceMappingRange* portalSurfaceMappingRange;
    struct DynamicBoxDefinition* dynamicBoxes;
    struct PortalSurfaceMappingRange* portalSurfaceDynamicMappingRange;
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
    struct PedestalDefinition* pedestals;
    struct SignageDefinition* signage;
    struct BoxDropperDefinition* boxDroppers;
    struct AnimationInfo* animations;
    struct SwitchDefinition* switches;
    struct BallLauncherDefinition* ballLaunchers;
    struct BallCatcherDefinition* ballCatchers;
    short collisionQuadCount;
    short staticContentCount;
    short portalSurfaceCount;
    short dynamicBoxCount;
    short triggerCount;
    short cutsceneCount;
    short locationCount;
    short doorCount;
    short buttonCount;
    short signalOperatorCount;
    short decorCount;
    short fizzlerCount;
    short elevatorCount;
    short pedestalCount;
    short signageCount;
    short boxDropperCount;
    short animationInfoCount;
    short switchCount;
    short ballLauncherCount;
    short ballCatcherCount;
    short startLocation;
};

#endif
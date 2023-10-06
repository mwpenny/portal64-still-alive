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
    CutsceneStepShowPrompt,
};

enum CutscenePromptType {
    CutscenePromptTypeNone,
    CutscenePromptTypePortal0,
    CutscenePromptTypePortal1,
    CutscenePromptTypePickup,
    CutscenePromptTypeDrop,
    CutscenePromptTypeUse,
    CutscenePromptTypeCrouch,
    CutscenePromptTypeMove,
    CutscenePromptTypeJump,
};

enum CutsceneSubtitleType {
    CutsceneSubtitleTypeNone,
    SUBTITLE_00_PART1_ENTRY_1,
    SUBTITLE_00_PART1_ENTRY_2,
    SUBTITLE_00_PART1_ENTRY_3,
    SUBTITLE_00_PART1_ENTRY_4,
    SUBTITLE_00_PART1_ENTRY_5,
    SUBTITLE_00_PART1_ENTRY_6,
    SUBTITLE_00_PART1_ENTRY_7,
    SUBTITLE_00_PART1_SUCCESS_1,
    SUBTITLE_00_PART1_SUCCESS_2,
    SUBTITLE_00_PART1_SUCCESS_3,
    SUBTITLE_00_PART2_ENTRY_1,
    SUBTITLE_00_PART2_SUCCESS_1,
    SUBTITLE_01_PART1_ENTRY_1,
    SUBTITLE_01_PART1_ENTRY_2,
    SUBTITLE_01_PART1_GET_PORTAL_GUN_1,
    SUBTITLE_01_PART1_GET_PORTAL_GUN_2,
    SUBTITLE_01_PART1_GET_PORTAL_GUN_3,
    SUBTITLE_01_PART1_GET_PORTAL_GUN_4,
    SUBTITLE_01_PART1_GET_PORTAL_GUN_5,
    SUBTITLE_01_PART1_GET_PORTAL_GUN_6,
    SUBTITLE_01_PART1_GET_PORTAL_GUN_7,
    SUBTITLE_01_PART1_GET_PORTAL_GUN_8,
    SUBTITLE_01_PART2_ENTRY_1,
    SUBTITLE_01_PART2_SUCCESS_1,
    SUBTITLE_02_PART1_ENTRY_1,
    SUBTITLE_02_PART1_ENTRY_2,
    SUBTITLE_02_PART1_SUCCESS_1,
    SUBTITLE_02_PART1_SUCCESS_2,
    SUBTITLE_ESCAPE_01_PART1_NAG01_1,
    SUBTITLE_02_PART2_SUCCESS_1,
    SUBTITLE_02_PART2_SUCCESS_2,
    SUBTITLE_03_PART2_ENTRY_1,
    SUBTITLE_03_PART2_PLATFORM_ACTIVATED_1,
    SUBTITLE_03_PART1_ENTRY_1,
    SUBTITLE_03_PART1_ENTRY_2,
    SUBTITLE_03_PART1_SUCCESS_1,
    SUBTITLE_04_PART1_SUCCESS_1,
    SUBTITLE_04_PART1_ENTRY_1,
    SUBTITLE_05_PART1_ENTRY_1,
    SUBTITLE_05_PART1_ENTRY_2,
    SUBTITLE_05_PART1_SUCCESS_1,
    SUBTITLE_05_PART1_NAG1_1,
    SUBTITLE_05_PART1_NAG2_1,
    SUBTITLE_05_PART1_NAG3_1,
    SUBTITLE_05_PART1_NAG4_1,
    SUBTITLE_05_PART1_NAG5_1,
    SUBTITLE_06_PART1_ENTRY_1,
    SUBTITLE_06_PART1_SUCCESS_2_1,
    SUBTITLE_06_PART1_SUCCESS_1_1,
    SUBTITLE_07_PART1_ENTRY_1,
    SUBTITLE_07_PART1_ENTRY_2,
    SUBTITLE_07_PART1_ENTRY_3,
    SUBTITLE_07_PART2_ENTRY_1,
    SUBTITLE_07_PART2_SUCCESS_1,
    SUBTITLE_07_PART1_GET_DEVICE_COMPONENT_1,
    SUBTITLE_07_PART1_GET_DEVICE_COMPONENT_2,
    SUBTITLE_07_PART1_GET_DEVICE_COMPONENT_3,
    SUBTITLE_07_PART1_TRAPPED_1,
    SUBTITLE_07_PART1_TRAPPED_2,
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
            u16 subtitleId;
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
        struct {
            u8 actionPromptType;
        } showPrompt;
        int noop;
    };
};

struct Cutscene {
    struct CutsceneStep* steps;
    u16 stepCount;
};

enum ObjectTriggerType {
    ObjectTriggerTypeNone,
    ObjectTriggerTypePlayer,
    ObjectTriggerTypeCube,
    ObjectTriggerTypeCubeHover,
};

struct ObjectTriggerInfo {
    short objectType;
    short cutsceneIndex;
    short signalIndex;
};

struct Trigger {
    struct Box3D box;
    struct ObjectTriggerInfo* triggers;
    short triggerCount;
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
    float ballVelocity;
};

struct BallCatcherDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
    short signalIndex;
};

struct ClockDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
    short cutsceneIndex;
};

struct SecurityCameraDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
};

struct LevelDefinition {
    struct CollisionObject* collisionQuads;
    struct StaticContentElement *staticContent;
    struct Rangeu16 *signalToStaticRanges;
    u16 *signalToStaticIndices;
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
    struct ClockDefinition* clocks;
    struct SecurityCameraDefinition* securityCameras;
    short collisionQuadCount;
    short staticContentCount;
    short signalToStaticCount;
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
    short clockCount;
    short securityCameraCount;
    short startLocation;
    short playerAnimatorIndex;
};

#endif
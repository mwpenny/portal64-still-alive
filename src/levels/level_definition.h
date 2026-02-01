#ifndef __LEVEL_DEFINITION_H__
#define __LEVEL_DEFINITION_H__

#include "effects/effect_definitions.h"
#include "math/box3d.h"
#include "math/boxs16.h"
#include "math/range.h"
#include "math/rotated_box.h"
#include "physics/collision_box.h"
#include "physics/world.h"
#include "scene/portal_surface.h"
#include "scene/signals.h"
#include "sk64/skeletool_animator.h"
#include "sk64/skeletool_armature.h"
#include "sk64/skeletool_clip.h"

#define NO_TRANSFORM_INDEX    0xFF
#define NO_BOUNDING_BOX_INDEX 0xFF

struct StaticContentBox {
    struct BoundingBoxs16 box;
    struct Rangeu16 staticRange;
    short siblingOffset;
};

struct StaticIndex {
    struct StaticContentBox* boxIndex;
    struct BoundingBoxs16* animatedBoxes;
    struct Rangeu16 animatedRange;
    short boxCount;
};

struct StaticContentElement {
    Gfx* displayList;
    struct Vector3 center;
    u8 materialIndex;
    u8 transformIndex;
    u8 boundingBoxIndex;
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
    CutsceneStepTypeDelayRandom,
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
    CutsceneStepDamagePlayer,
    CutsceneStepTypeClosePortal,
    CutsceneStepShowPrompt,
    CutsceneStepRumble,
    CutsceneStepActivateSignage,
    CutsceneStepPlayEffect,
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



#define CH_NONE    0xFF
#define CH_GLADOS  0
#define CH_MUSIC   1
#define CH_AMBIENT 2

#define CH_COUNT   3

struct CutsceneStep {
    enum CutsceneStepType type;

    union {
        struct {
            u16 soundId;
            u8 volume;
            u8 pitch;
            s16 locationIndex;
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
            u8 portalIndex;
            u8 fromPedestal;
        } openPortal;
        struct {
            u16 portalIndex;
        } closePortal;
        float delay;
        struct {
            float min;
            float max;
        } delayRandom;
        struct {
            u16 signalIndex;
            u16 signalValue;
        } setSignal;
        struct {
            u16 signalIndex;
            u8 forFrames;
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
            u8 playShootingSound;
        } pointPedestal;
        struct {
            u8 armatureIndex;
            u8 animationIndex;
            s8 playbackSpeed;
            u8 flags;
        } playAnimation;
        struct {
            u8 armatureIndex;
            s8 playbackSpeed;
        } setAnimationSpeed;
        struct {
            u8 armatureIndex;
        } waitForAnimation;
        struct {
            float amount;
        } damagePlayer;
        struct {
            u8 actionPromptType;
        } showPrompt;
        struct {
            u8 rumbleLevel;
        } rumble;
        struct {
            u8 testChamberIndex;
        } activateSignage;
        struct {
            u8 effectIndex;
            u16 locationIndex;
        } playEffect;
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

enum TriggerType {
    TriggerTypeContain,
    TriggerTypeTouch
};

struct Trigger {
    struct Box3D box;
    struct ObjectTriggerInfo* triggers;
    short triggerCount;
    enum TriggerType type;
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

struct DoorwayCoverDefinition {
    struct Vector3 position;
    struct Basis basis;
    float fadeStartDistance;
    float fadeEndDistance;
    struct Coloru8 color;
    short doorwayIndex;
};

struct ButtonDefinition {
    struct Vector3 location;
    short roomIndex;
    short signalIndex;
    short objectSignalIndex;
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
    u8 startAsleep;
};

struct FizzlerDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    float width;
    float height;
    short roomIndex;
    short cubeSignalIndex;
};

struct ElevatorDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
    short targetElevator;
};

struct PedestalDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
};

struct SignageDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
    short testChamberNumber;
};

enum BoxDropperCubeType {
    BoxDropperCubeTypeNone,
    BoxDropperCubeTypeStandard,
};

struct BoxDropperDefinition {
    struct Vector3 position;
    short roomIndex;
    short signalIndex;
    enum BoxDropperCubeType cubeType;
};

enum AnimationSoundType {
    AnimationSoundTypeNone,
    AnimationSoundTypeLightRail,
    AnimationSoundTypePiston,
    AnimationSoundTypeArm,
    AnimationSoundTypeStairs,
    AnimationSoundTypeDoor
};

struct AnimationInfo {
    struct SKArmatureDefinition armature;
    struct SKAnimationClip* clips;
    short clipCount;
    short soundType;
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
    float duration;
};

struct SecurityCameraDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
};

struct TurretDefinition {
    struct Vector3 position;
    struct Quaternion rotation;
    short roomIndex;
    short playerCanAutotip;
};

struct LevelDefinition {
    struct CollisionObject* collisionQuads;
    struct StaticContentElement* staticContent;
    // For precise culling (second pass after BVH)
    struct RotatedBox* staticBoundingBoxes;
    struct StaticIndex* roomBvhList;
    struct Rangeu16* signalToStaticRanges;
    u16* signalToStaticIndices;
    struct Rangeu16* roomStaticMapping;
    struct PortalSurface* portalSurfaces;
    // Maps index of a collisionQuads to indices in portalSurfaces
    struct PortalSurfaceMappingRange* portalSurfaceMappingRange;
    struct DynamicBoxDefinition* dynamicBoxes;
    struct PortalSurfaceMappingRange* portalSurfaceDynamicMappingRange;
    u8* portalSurfaceMappingIndices;
    struct Trigger* triggers;
    struct Cutscene* cutscenes;
    struct Location* locations;
    struct World world;
    struct DoorDefinition* doors;
    struct DoorwayCoverDefinition* doorwayCovers;
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
    struct TurretDefinition* turrets;
    short collisionQuadCount;
    short staticContentCount;
    short signalToStaticCount;
    short portalSurfaceCount;
    short dynamicBoxCount;
    short triggerCount;
    short cutsceneCount;
    short locationCount;
    short doorCount;
    short doorwayCoverCount;
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
    short turretCount;
    short startLocation;
    short playerAnimatorIndex;
};

#endif
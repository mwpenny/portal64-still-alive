#include "grab_rotation.h"
#include "../player/player.h"


struct Vector3 gCubeSurfaceNormals[6] = {
    {  1.0f,  0.0f,  0.0f },
    { -1.0f,  0.0f,  0.0f },
    {  0.0f,  1.0f,  0.0f },
    {  0.0f, -1.0f,  0.0f },
    {  0.0f,  0.0f,  1.0f },
    {  0.0f,  0.0f, -1.0f }
};


enum GrabRotationFlags grabRotationFlagsForDecorId(const int decorId) {
    enum GrabRotationFlags flags = 0;
    // object specific flags
    if (decorId == DECOR_TYPE_RADIO) {
        flags |= GrabRotationTurnTowardsPlayer | GrabRotationUseZLookDirection;
    }
    else // default flags
    {
        flags |= GrabRotationSnapToCubeNormals;
    }
    return flags;
}

enum GrabRotationFlags grabRotationFlagsForDecorObjectDef(struct DecorObjectDefinition* decorObjectDef) {
    return grabRotationFlagsForDecorId(decorIdForObjectDefinition(decorObjectDef));
}

enum GrabRotationFlags grabRotationFlagsForCollisionObject(struct CollisionObject* collisionObject) {
    return grabRotationFlagsForDecorId(decorIdForCollisionObject(collisionObject)); // this will probably need to be replaced in the future
}


void grabRotationApplyTurnTowardsPlayer(struct Quaternion* grabRotationBaseOut) {
    quatIdent(grabRotationBaseOut);
}

void grabRotationApplySnapToCubeNormals(struct Quaternion* forwardRotationIn, struct Quaternion* objectRotationIn, struct Quaternion* grabRotationBaseOut) {
    struct Vector3 forward, up;
    quatMultVector(forwardRotationIn, &gForward, &forward);
    quatMultVector(forwardRotationIn, &gUp, &up);
    
    int closestNormalTowards = 0, closestNormalUp = 0;
    float closestNormalTowardsDot = 1.0f, closestNormalUpDot = -1.0f;
    for (int i = 0; i < 6; ++i) {
        struct Vector3 surfaceNormal;
        quatMultVector(objectRotationIn, &gCubeSurfaceNormals[i], &surfaceNormal);
        
        float dot = vector3Dot(&surfaceNormal, &forward);
        if (dot < closestNormalTowardsDot) {
            closestNormalTowardsDot = dot;
            closestNormalTowards = i;
        }
        dot = vector3Dot(&surfaceNormal, &up);
        if (dot > closestNormalUpDot) {
            closestNormalUpDot = dot;
            closestNormalUp = i;
        }
    }
    struct Quaternion normalRotation;
    quatLook(&gCubeSurfaceNormals[closestNormalTowards], &gCubeSurfaceNormals[closestNormalUp], &normalRotation);
    quatConjugate(&normalRotation, grabRotationBaseOut);
}

void grabRotationApplyNoRotation(struct Quaternion* forwardRotationIn, struct Quaternion* objectRotationIn, struct Quaternion* grabRotationBaseOut) {
    struct Quaternion forwardRotationInverted;
    quatConjugate(forwardRotationIn, &forwardRotationInverted);
    quatMultiply(&forwardRotationInverted, objectRotationIn, grabRotationBaseOut);
}

void grabRotationInitBase(const enum GrabRotationFlags flags, struct Quaternion* forwardRotationIn, struct Quaternion* objectRotationIn, struct Quaternion* grabRotationBaseOut) {
    // modify object rotation according to flags
    if (flags & GrabRotationTurnTowardsPlayer) {
        grabRotationApplyTurnTowardsPlayer(grabRotationBaseOut);
    } else if (flags & GrabRotationSnapToCubeNormals) {
        grabRotationApplySnapToCubeNormals(forwardRotationIn, objectRotationIn, grabRotationBaseOut);
    } else { // with no rotation modifier, object is not rotated on grab
        grabRotationApplyNoRotation(forwardRotationIn, objectRotationIn, grabRotationBaseOut);
    }
}


void grabRotationApplyUseZLookDirection(struct Quaternion* lookRotationDeltaIn, struct Quaternion* grabRotationBaseInOut) {
    struct Quaternion tmp;
    quatMultiply(lookRotationDeltaIn, grabRotationBaseInOut, &tmp);
    *grabRotationBaseInOut = tmp;
}

void grabRotationUpdate(const enum GrabRotationFlags flags, struct Quaternion* lookRotationDeltaIn, struct Quaternion* forwardRotationIn, struct Quaternion* grabRotationBaseIn, struct Quaternion* grabRotationOut) {
    // modify target object rotation in object-space
    struct Quaternion grabRotationBase = *grabRotationBaseIn;
    if (flags & GrabRotationUseZLookDirection) {
        grabRotationApplyUseZLookDirection(lookRotationDeltaIn, &grabRotationBase);
    }
    
    // maintain object's relative rotation
    quatMultiply(forwardRotationIn, &grabRotationBase, grabRotationOut);
}

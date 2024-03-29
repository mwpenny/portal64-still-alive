#include "grab_rotation.h"
#include "../player/player.h"


enum GrabRotationFlags grabRotationFlagsForDecorId(const int decorId) {
    enum GrabRotationFlags flags = 0;
    // default flags
    flags |= GrabRotationSnapToCubeNormals;
    // object specific flags
    if (decorId == DECOR_TYPE_RADIO) {
        flags &= ~GrabRotationSnapToCubeNormals;
        flags |= GrabRotationTurnTowardsPlayer | GrabRotationUseZLookDirection;
    }
    return flags;
}

enum GrabRotationFlags grabRotationFlagsForDecorObjectDef(struct DecorObjectDefinition* decorObjectDef) {
    return grabRotationFlagsForDecorId(decorIdForObjectDefinition(decorObjectDef));
}

enum GrabRotationFlags grabRotationFlagsForCollisionObject(struct CollisionObject* collisionObject) {
    return grabRotationFlagsForDecorId(decorIdForCollisionObject(collisionObject)); // this will probably need to be replaced in the future
}


void grabRotationApplySnapToCubeNormals(const enum GrabRotationFlags flags, struct Vector3* forwardDirIn, struct Quaternion* forwardRotationIn, struct Quaternion* objectRotationInOut) {
    if (!(flags & GrabRotationSnapToCubeNormals) || (flags & GrabRotationTurnTowardsPlayer)) {
        return; // GrabRotationTurnTowardsPlayer takes precedence anyways if enabled
    }
    struct Vector3 surfaceNormal, closestNormal;
    float closestNormalDot = 1.0f; // has to be negative on success
    for (int i = 0; i < 6; ++i) {
        if      (i == 0) { surfaceNormal = (struct Vector3){ 1.0f,  0.0f,  0.0f}; }
        else if (i == 1) { surfaceNormal = (struct Vector3){-1.0f,  0.0f,  0.0f}; }
        else if (i == 2) { surfaceNormal = (struct Vector3){ 0.0f,  1.0f,  0.0f}; }
        else if (i == 3) { surfaceNormal = (struct Vector3){ 0.0f, -1.0f,  0.0f}; }
        else if (i == 4) { surfaceNormal = (struct Vector3){ 0.0f,  0.0f,  1.0f}; }
        else if (i == 5) { surfaceNormal = (struct Vector3){ 0.0f,  0.0f, -1.0f}; }
        quatMultVector(objectRotationInOut, &surfaceNormal, &surfaceNormal);
        
        float normalDot = vector3Dot(&surfaceNormal, forwardDirIn);
        if (normalDot < 0.0f && normalDot < closestNormalDot) {
            closestNormalDot = normalDot;
            closestNormal = surfaceNormal;
        }
    }
    if (closestNormalDot < 0.0f) {
        struct Quaternion surfaceRotation;
        quatLook(&closestNormal, &gUp, &surfaceRotation);
        struct Quaternion objectRotationOriginal = *objectRotationInOut;
        struct Quaternion forwardRotationInv;
        quatConjugate(forwardRotationIn, &forwardRotationInv);
        quatMultiply(&forwardRotationInv, &surfaceRotation, objectRotationInOut);
        quatConjugate(objectRotationInOut, &surfaceRotation);
        quatMultiply(&surfaceRotation, &objectRotationOriginal, objectRotationInOut);
    }
}

void grabRotationApplyTurnTowardsPlayer(const enum GrabRotationFlags flags, struct Quaternion* forwardRotationIn, struct Quaternion* objectRotationInOut) {
    if (!(flags & GrabRotationTurnTowardsPlayer)) {
        return;
    }
    *objectRotationInOut = *forwardRotationIn;
}

void grabRotationApplyUseZLookDirection(const enum GrabRotationFlags flags, struct Quaternion* lookRotationDeltaIn, struct Quaternion* grabRotationBaseIn, struct Quaternion* grabRotationBaseOut) {
    if (!(flags & GrabRotationUseZLookDirection)) {
        return;
    }
    quatMultiply(lookRotationDeltaIn, grabRotationBaseIn, grabRotationBaseOut);
}

void grabRotationGetBase(const enum GrabRotationFlags flags, struct Quaternion* objectRotationIn, struct Quaternion* lookRotationIn, struct Quaternion* grabRotationBaseOut) {
    struct Quaternion forwardRotation;
    struct Vector3 forward, tmpVec;
    playerGetMoveBasis(lookRotationIn, &forward, &tmpVec);
    vector3Negate(&forward, &tmpVec);
    quatLook(&tmpVec, &gUp, &forwardRotation);
    
    // modify object rotation according to flags
    struct Quaternion objectRotation = *objectRotationIn;
    grabRotationApplySnapToCubeNormals(flags, &forward, &forwardRotation, &objectRotation);
    grabRotationApplyTurnTowardsPlayer(flags, &forwardRotation, &objectRotation);
    
    // untangle objectRotation from relative forwardRotation, store as grabRotationBase
    quatConjugate(&forwardRotation, &forwardRotation);
    quatMultiply(&forwardRotation, &objectRotation, grabRotationBaseOut);
}

void grabRotationUpdate(const enum GrabRotationFlags flags, struct Quaternion* lookRotationDeltaIn, struct Quaternion* forwardRotationIn, struct Quaternion* grabRotationBaseIn, struct Quaternion* grabRotationOut) {
    // modify target object rotation in object-space
    struct Quaternion grabRotationBase;
    grabRotationApplyUseZLookDirection(flags, lookRotationDeltaIn, grabRotationBaseIn, &grabRotationBase);
    
    // maintain object's relative rotation
    quatMultiply(forwardRotationIn, &grabRotationBase, grabRotationOut);
}

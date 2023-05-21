#include "collision_object.h"
#include "epa.h"
#include "gjk.h"
#include "contact_insertion.h"
#include "collision_scene.h"
#include "../math/mathf.h"
#include "mesh_collider.h"
// 0x807572ac
void collisionObjectInit(struct CollisionObject* object, struct ColliderTypeData *collider, struct RigidBody* body, float mass, int collisionLayers) {
    object->collider = collider;
    object->body = body;
    rigidBodyInit(body, mass, collider->callbacks->mofICalculator(collider, mass));
    collisionObjectUpdateBB(object);
    object->collisionLayers = collisionLayers;
    object->flags = 0;
    object->data = 0;
    object->trigger = 0;
    object->manifoldIds = 0;
}

void collisionObjectReInit(struct CollisionObject* object, struct ColliderTypeData *collider, struct RigidBody* body, float mass, int collisionLayers) {
    object->collider = collider;
    object->body = body;
    collisionObjectUpdateBB(object);
}

int collisionObjectIsActive(struct CollisionObject* object) {
    return object->body && ((object->body->flags & (RigidBodyIsKinematic | RigidBodyIsSleeping)) == 0);
}

int collisionObjectIsGrabbable(struct CollisionObject* object) {
    return object->body && ((object->body->flags & (RigidBodyFlagsGrabbable)) != 0);
}

int collisionObjectShouldGenerateConctacts(struct CollisionObject* object) {
    return collisionObjectIsActive(object) || (object->body->flags & RigidBodyIsPlayer) != 0;
}

void collisionObjectCollideWithQuad(struct CollisionObject* object, struct CollisionObject* quadObject, struct ContactSolver* contactSolver) {
    if ((object->collisionLayers & quadObject->collisionLayers) == 0) {
        return;
    }

    if (!box3DHasOverlap(&object->boundingBox, &quadObject->boundingBox)) {
        return;
    }

    if (object->trigger && quadObject->trigger) {
        return;
    }

    struct Simplex simplex;

    struct CollisionQuad* quad = (struct CollisionQuad*)quadObject->collider->data;

    if (!gjkCheckForOverlap(&simplex, 
                quad, minkowsiSumAgainstQuad, 
                object, minkowsiSumAgainstObject, 
                &quad->plane.normal)) {
        return;
    }

    if (object->trigger) {
        object->trigger(object->data, quadObject);
        return;
    }

    if (quadObject->trigger) {
        quadObject->trigger(quadObject->data, object);
        return;
    }

    struct EpaResult result;
    epaSolve(
        &simplex, 
        quad, minkowsiSumAgainstQuad, 
        object, minkowsiSumAgainstObject, 
        &result
    );

    int touchingPortals = collisionSceneIsTouchingPortal(&result.contactA, &result.normal);

    if (touchingPortals) {
        object->body->flags |= touchingPortals;
        return;
    }

    struct ContactManifold* contact = contactSolverGetContactManifold(contactSolver, quadObject, object);

    if (!contact) {
        return;
    }

    contact->friction = MAX(object->collider->friction, quadObject->collider->friction);
    contact->restitution = MIN(object->collider->bounce, quadObject->collider->bounce);

    transformPointInverseNoScale(&object->body->transform, &result.contactB, &result.contactB);
    contactInsert(contact, &result);
}

void collisionObjectHandleSweptCollision(struct CollisionObject* object, struct Vector3* normal, float restitution) {
    float velocityDot = vector3Dot(normal, &object->body->velocity);

    if (velocityDot < 0.0f) {
        vector3AddScaled(&object->body->velocity, normal, (1 + restitution) * -velocityDot, &object->body->velocity);
        vector3AddScaled(&object->body->transform.position, normal, -0.01f, &object->body->transform.position);
    }
}

void collisionObjectCollideWithQuadSwept(struct CollisionObject* object, struct Vector3* objectPrevPos, struct Box3D* sweptBB, struct CollisionObject* quadObject, struct ContactSolver* contactSolver) {
    if ((object->collisionLayers & quadObject->collisionLayers) == 0) {
        return;
    }

    if (!box3DHasOverlap(sweptBB, &quadObject->boundingBox)) {
        return;
    }

    if (object->trigger && quadObject->trigger) {
        return;
    }

    struct Simplex simplex;

    struct CollisionQuad* quad = (struct CollisionQuad*)quadObject->collider->data;

    struct SweptCollisionObject sweptObject;
    sweptObject.object = object;
    sweptObject.prevPos = objectPrevPos;

    if (!gjkCheckForOverlap(&simplex, 
                quad, minkowsiSumAgainstQuad, 
                &sweptObject, minkowsiSumAgainstSweptObject, 
                &quad->plane.normal)) {
        return;
    }

    if (object->trigger) {
        object->trigger(object->data, quadObject);
        return;
    }

    if (quadObject->trigger) {
        quadObject->trigger(quadObject->data, object);
        return;
    }

    struct EpaResult result;
    struct Vector3 objectEnd = object->body->transform.position;

    if (!epaSolveSwept(
        &simplex, 
        quad, minkowsiSumAgainstQuad, 
        &sweptObject, minkowsiSumAgainstSweptObject,
        objectPrevPos,
        &objectEnd,
        &result
    )) {
        collisionObjectCollideWithQuad(object, quadObject, contactSolver);
        return;
    }

    // quads with a thickness of 0 are one sided
    if (quad->thickness == 0.0f && vector3Dot(&result.normal, &quad->plane.normal) < 0.0f) {
        return;
    }

    int touchingPortals = collisionSceneIsTouchingPortal(&result.contactA, &result.normal);

    if (touchingPortals) {
        object->body->flags |= touchingPortals;
        return;
    }

    struct ContactManifold* contact = contactSolverGetContactManifold(contactSolver, quadObject, object);

    if (!contact) {
        return;
    }

    object->body->transform.position = objectEnd;

    contact->friction = MIN(object->collider->friction, quadObject->collider->friction);
    contact->restitution = MAX(object->collider->bounce, quadObject->collider->bounce);

    transformPointInverseNoScale(&object->body->transform, &result.contactB, &result.contactB);
    contactInsert(contact, &result);

    collisionObjectHandleSweptCollision(object, &contact->normal, contact->restitution);
}


void collisionObjectCollideTwoObjects(struct CollisionObject* a, struct CollisionObject* b, struct ContactSolver* contactSolver) {
    if (!box3DHasOverlap(&a->boundingBox, &b->boundingBox)) {
        return;
    }

    if (a->trigger && b->trigger) {
        return;
    }

    if (a->collider->type == CollisionShapeTypeMesh) {
        if (b->collider->type != CollisionShapeTypeMesh) {
            meshColliderCollideObject(a, b, contactSolver);
        }

        return;
    } else if (b->collider->type == CollisionShapeTypeMesh) {
        meshColliderCollideObject(b, a, contactSolver);
        return;
    }

    struct Simplex simplex;

    struct Vector3 offset;

    vector3Sub(&b->body->transform.position, &a->body->transform.position, &offset);

    if (!gjkCheckForOverlap(&simplex, 
                a, minkowsiSumAgainstObject, 
                b, minkowsiSumAgainstObject, 
                &offset)) {
        return;
    }

    if (a->trigger) {
        a->trigger(a->data, b);
        return;
    }

    if (b->trigger) {
        b->trigger(b->data, a);
        return;
    }

    struct EpaResult result;
    epaSolve(
        &simplex, 
        a, minkowsiSumAgainstObject, 
        b, minkowsiSumAgainstObject, 
        &result
    );

    int touchingPortals = collisionSceneIsTouchingPortal(&result.contactA, &result.normal);

    if (touchingPortals) {
        b->body->flags |= touchingPortals;
        return;
    }

    struct Vector3 minusNormal;
    vector3Negate(&result.normal, &minusNormal);
    
    touchingPortals = collisionSceneIsTouchingPortal(&result.contactB, &minusNormal);

    if (touchingPortals) {
        a->body->flags |= touchingPortals;
        return;
    }


    struct ContactManifold* contact = contactSolverGetContactManifold(contactSolver, a, b);

    if (!contact) {
        return;
    }

    transformPointInverseNoScale(&a->body->transform, &result.contactA, &result.contactA);
    transformPointInverseNoScale(&b->body->transform, &result.contactB, &result.contactB);

    if (contact->shapeA == b) {
        epaSwapResult(&result);
    }

    contact->friction = MAX(a->collider->friction, b->collider->friction);
    contact->restitution = MIN(a->collider->bounce, b->collider->bounce);

    contactInsert(contact, &result);
}


void collisionObjectCollideTwoObjectsSwept(
    struct CollisionObject* a, 
    struct Vector3* prevAPos, 
    struct Box3D* sweptA,
    struct CollisionObject* b, 
    struct Vector3* prevBPos, 
    struct Box3D* sweptB,
    struct ContactSolver* contactSolver
) {
    if (!box3DHasOverlap(sweptA, sweptB)) {
        return;
    }

    if (a->trigger && b->trigger) {
        return;
    }

    if (a->collider->type == CollisionShapeTypeMesh) {
        if (b->collider->type != CollisionShapeTypeMesh) {
            meshColliderCollideObject(a, b, contactSolver);
        }

        return;
    } else if (b->collider->type == CollisionShapeTypeMesh) {
        meshColliderCollideObject(b, a, contactSolver);
        return;
    }

    struct Simplex simplex;

    struct Vector3 relativePrevPos;
    vector3Sub(prevBPos, prevAPos, &relativePrevPos);
    vector3Add(&relativePrevPos, &a->body->transform.position, &relativePrevPos);

    struct SweptCollisionObject sweptObject;
    sweptObject.object = b;
    sweptObject.prevPos = prevBPos;

    if (!gjkCheckForOverlap(&simplex, 
                a, minkowsiSumAgainstObject, 
                &sweptObject, minkowsiSumAgainstSweptObject, 
                &relativePrevPos)) {
        return;
    }

    if (a->trigger) {
        a->trigger(a->data, b);
        return;
    }

    if (b->trigger) {
        b->trigger(b->data, a);
        return;
    }

    struct EpaResult result;

    struct Vector3 objectEnd = b->body->transform.position;
    
    if (!epaSolveSwept(
        &simplex, 
        a, minkowsiSumAgainstObject, 
        &sweptObject, minkowsiSumAgainstSweptObject,
        &relativePrevPos,
        &objectEnd,
        &result
    )) {
        collisionObjectCollideTwoObjects(a, b, contactSolver);
        return;
    }

    // determine how long each object travelled before colliding
    struct Vector3 moveAmount;
    struct Vector3 relativeOffset;

    vector3Sub(&objectEnd, &relativePrevPos, &moveAmount);
    vector3Sub(&b->body->transform.position, &relativePrevPos, &relativeOffset);
    float lerpAmount;

    float relativeMovementSqrd = vector3MagSqrd(&relativeOffset);

    if (relativeMovementSqrd < 0.0000001f) {
        lerpAmount = 0.0f;
    } else {
        lerpAmount = vector3Dot(&moveAmount, &relativeOffset) / relativeMovementSqrd;
    }

    // update the points of contact
    struct Vector3 contactOffset;
    vector3Sub(&result.contactA, &objectEnd, &contactOffset);
    vector3Add(&objectEnd, &contactOffset, &result.contactA);
    result.contactB = result.contactA;

    // determine if a portal was hit
    int touchingPortals = collisionSceneIsTouchingPortal(&result.contactA, &result.normal);

    if (touchingPortals) {
        b->body->flags |= touchingPortals;
        return;
    }

    struct Vector3 minusNormal;
    vector3Negate(&result.normal, &minusNormal);
    
    touchingPortals = collisionSceneIsTouchingPortal(&result.contactB, &minusNormal);

    if (touchingPortals) {
        a->body->flags |= touchingPortals;
        return;
    }

    // update both objects to where they collided
    if (!(a->body->flags & RigidBodyIsKinematic)) {
        vector3Lerp(prevAPos, &a->body->transform.position, lerpAmount, &a->body->transform.position);
    }
    if (!(b->body->flags & RigidBodyIsKinematic)) {
        vector3Lerp(prevBPos, &b->body->transform.position, lerpAmount, &b->body->transform.position);
    }

    struct ContactManifold* contact = contactSolverGetContactManifold(contactSolver, a, b);

    if (!contact) {
        return;
    }

    transformPointInverseNoScale(&a->body->transform, &result.contactA, &result.contactA);
    transformPointInverseNoScale(&b->body->transform, &result.contactB, &result.contactB);

    if (contact->shapeA == b) {
        epaSwapResult(&result);
    }

    contact->friction = MIN(a->collider->friction, b->collider->friction);
    contact->restitution = MAX(a->collider->bounce, b->collider->bounce);

    contactInsert(contact, &result);

    struct Vector3 normalReverse;
    vector3Negate(&contact->normal, &normalReverse);
    collisionObjectHandleSweptCollision(a, &normalReverse, contact->restitution);
    collisionObjectHandleSweptCollision(b, &contact->normal, contact->restitution);
}

void collisionObjectUpdateBB(struct CollisionObject* object) {
    if (object->body) {
        object->collider->callbacks->boundingBoxCalculator(object->collider, &object->body->transform, &object->boundingBox);
        basisFromQuat(&object->body->rotationBasis, &object->body->transform.rotation);
    }
}

int minkowsiSumAgainstQuad(void* data, struct Vector3* direction, struct Vector3* output) {
    struct CollisionQuad* quad = (struct CollisionQuad*)data;
    *output = quad->corner;

    int result = 0;

    if (vector3Dot(&quad->edgeA, direction) > 0.0f) {
        vector3AddScaled(output, &quad->edgeA, quad->edgeALength, output);
        result |= 0x1;
    } else {
        result |= 0x2;
    }

    if (vector3Dot(&quad->edgeB, direction) > 0.0f) {
        vector3AddScaled(output, &quad->edgeB, quad->edgeBLength, output);
        result |= 0x4;
    } else {
        result |= 0x8;
    }

    if (quad->thickness > 0.0f) {
        if (vector3Dot(&quad->plane.normal, direction) < 0.0f) {
            vector3AddScaled(output, &quad->plane.normal, -quad->thickness, output);
            result |= 0x10;
        } else {
            result |= 0x20;
        }
    } else {
        result |= 0x20;
    }

    return result;
}

int minkowsiSumAgainstSweptObject(void* data, struct Vector3* direction, struct Vector3* output) {
    struct SweptCollisionObject* sweptObject = (struct SweptCollisionObject*)data;

    struct ColliderTypeData* collider = sweptObject->object->collider;
    struct RigidBody* body = sweptObject->object->body;

    int result = collider->callbacks->minkowsiSum(
        collider->data, 
        &body->rotationBasis, 
        direction, 
        output
    );

    if (vector3Dot(&body->transform.position, direction) > vector3Dot(sweptObject->prevPos, direction)) {
        vector3Add(output, &body->transform.position, output);
    } else {
        vector3Add(output, sweptObject->prevPos, output);
    }

    return result;
}

int minkowsiSumAgainstObject(void* data, struct Vector3* direction, struct Vector3* output) {
    struct CollisionObject* object = (struct CollisionObject*)data;
    int result = object->collider->callbacks->minkowsiSum(object->collider->data, &object->body->rotationBasis, direction, output);
    vector3Add(output, &object->body->transform.position, output);
    return result;
}

void collisionObjectLocalRay(struct CollisionObject* cylinderObject, struct Ray* ray, struct Ray* localRay) {
    struct Vector3 offset;
    vector3Sub(&ray->origin, &cylinderObject->body->transform.position, &offset);
    basisUnRotate(&cylinderObject->body->rotationBasis, &ray->dir, &localRay->dir);
    basisUnRotate(&cylinderObject->body->rotationBasis, &offset, &localRay->origin);
}
#include "collision_object.h"

#include <assert.h>

#include "collision_scene.h"
#include "compound_collider.h"
#include "contact_insertion.h"
#include "epa.h"
#include "gjk.h"
#include "math/mathf.h"
#include "mesh_collider.h"

void collisionObjectInit(struct CollisionObject* object, struct ColliderTypeData *collider, struct RigidBody* body, float mass, int collisionLayers) {
    object->collider = collider;
    object->body = body;
    object->position = &body->transform.position;
    rigidBodyInit(body, mass, collider->callbacks->mofICalculator(collider, mass));
    collisionObjectUpdateBB(object);
    object->collisionLayers = collisionLayers;
    object->flags = 0;
    object->data = NULL;
    object->trigger = NULL;
    object->sweptCollide = NULL;
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

int collisionObjectShouldGenerateContacts(struct CollisionObject* object) {
    return collisionObjectIsActive(object) || (object->body->flags & RigidBodyIsPlayer) != 0;
}

struct ContactManifold* collisionObjectCollideWithQuad(struct CollisionObject* object, struct CollisionObject* quadObject, struct ContactSolver* contactSolver, int shouldCheckPortals) {
    if ((object->collisionLayers & quadObject->collisionLayers) == 0) {
        return NULL;
    }

    if (!box3DHasOverlap(&object->boundingBox, &quadObject->boundingBox)) {
        return NULL;
    }

    if (object->trigger && quadObject->trigger) {
        return NULL;
    }

    struct Simplex simplex;

    struct CollisionQuad* quad = (struct CollisionQuad*)quadObject->collider->data;

    if (!gjkCheckForOverlap(&simplex,
                quad, quadMinkowskiSupport,
                object, objectMinkowskiSupport,
                &quad->plane.normal)) {
        return NULL;
    }

    if (object->trigger) {
        object->trigger(object, quadObject);
        return NULL;
    }

    if (quadObject->trigger) {
        quadObject->trigger(quadObject, object);
        return NULL;
    }

    struct EpaResult result;
    epaSolve(
        &simplex,
        quad, quadMinkowskiSupport,
        object, objectMinkowskiSupport,
        &result
    );

    // quads with a thickness of 0 are one sided
    if (quad->thickness == 0.0f && vector3Dot(&result.normal, &quad->plane.normal) < 0.0f) {
        return SweptCollideResultMiss;
    }

    if (shouldCheckPortals) {
        int touchingPortals = collisionSceneIsTouchingPortal(&result.contactA, &result.normal);
        if (touchingPortals) {
            object->body->flags |= touchingPortals;
            return NULL;
        }
    }

    struct ContactManifold* contact = contactSolverGetContactManifold(contactSolver, quadObject, object);

    if (!contact) {
        return NULL;
    }

    contact->friction = MAX(object->collider->friction, quadObject->collider->friction);
    contact->restitution = MIN(object->collider->bounce, quadObject->collider->bounce);

    transformPointInverseNoScale(&object->body->transform, &result.contactB, &result.contactB);
    contactInsert(contact, &result);

    return contact;
}

void collisionObjectHandleSweptCollision(struct CollisionObject* object, struct Vector3* normal, float restitution) {
    float velocityDot = vector3Dot(normal, &object->body->velocity);

    if (velocityDot < 0.0f) {
        vector3AddScaled(&object->body->velocity, normal, (1 + restitution) * -velocityDot, &object->body->velocity);
        vector3AddScaled(&object->body->transform.position, normal, -0.01f, &object->body->transform.position);

        if (object->sweptCollide) {
            object->sweptCollide(object, -velocityDot);
        }
    }
}

enum SweptCollideResult collisionObjectSweptCollide(
    struct CollisionObject* object, 
    struct Vector3* objectPrevPos, 
    struct Box3D* sweptBB, 
    struct CollisionObject* quadObject, 
    int shouldCheckPortals, 
    struct EpaResult* result, 
    struct Vector3* objectEnd
) {
    if ((object->collisionLayers & quadObject->collisionLayers) == 0) {
        return SweptCollideResultMiss;
    }

    if (!box3DHasOverlap(sweptBB, &quadObject->boundingBox)) {
        return SweptCollideResultMiss;
    }

    if (object->trigger && quadObject->trigger) {
        return SweptCollideResultMiss;
    }

    struct Simplex simplex;

    struct CollisionQuad* quad = (struct CollisionQuad*)quadObject->collider->data;

    struct Vector3 offsetPrevPos = *objectPrevPos;
    collisionObjectAddBodyOffset(object, &offsetPrevPos);

    struct SweptCollisionObject sweptObject;
    sweptObject.object = object;
    sweptObject.prevPos = &offsetPrevPos;

    if (!gjkCheckForOverlap(&simplex,
                quad, quadMinkowskiSupport,
                &sweptObject, sweptObjectMinkowskiSupport,
                &quad->plane.normal)) {
        return SweptCollideResultMiss;
    }

    if (object->trigger) {
        object->trigger(object->data, quadObject);
        return SweptCollideResultMiss;
    }

    if (quadObject->trigger) {
        quadObject->trigger(quadObject->data, object);
        return SweptCollideResultMiss;
    }

    *objectEnd = object->body->transform.position;

    if (!epaSolveSwept(
        &simplex,
        quad, quadMinkowskiSupport,
        &sweptObject, sweptObjectMinkowskiSupport,
        objectPrevPos,
        objectEnd,
        result
    )) {
        return SweptCollideResultOverlap;
    }

    // quads with a thickness of 0 are one sided
    if (quad->thickness == 0.0f && vector3Dot(&result->normal, &quad->plane.normal) < 0.0f) {
        return SweptCollideResultMiss;
    }

    if (shouldCheckPortals) {
        int touchingPortals = collisionSceneIsTouchingPortal(&result->contactA, &result->normal);

        if (touchingPortals) {
            object->body->flags |= touchingPortals;
            return SweptCollideResultMiss;
        }
    }

    return SweptCollideResultHit;
}

struct ContactManifold* collisionObjectCollideWithQuadSwept(struct CollisionObject* object, struct Vector3* objectPrevPos, struct Box3D* sweptBB, struct CollisionObject* quadObject, struct ContactSolver* contactSolver, int shouldCheckPortals) {
    struct EpaResult result;
    struct Vector3 objectEnd;

    enum SweptCollideResult collideResult = collisionObjectSweptCollide(object, objectPrevPos, sweptBB, quadObject, shouldCheckPortals, &result, &objectEnd);

    if (collideResult == SweptCollideResultOverlap) {
        return collisionObjectCollideWithQuad(object, quadObject, contactSolver, shouldCheckPortals);
    }

    if (collideResult == SweptCollideResultMiss) {
        return NULL;
    }

    struct ContactManifold* contact = contactSolverGetContactManifold(contactSolver, quadObject, object);

    if (!contact) {
        return NULL;
    }

    object->body->transform.position = objectEnd;

    contact->friction = MIN(object->collider->friction, quadObject->collider->friction);
    contact->restitution = MAX(object->collider->bounce, quadObject->collider->bounce);

    transformPointInverseNoScale(&object->body->transform, &result.contactB, &result.contactB);
    contactInsert(contact, &result);

    collisionObjectHandleSweptCollision(object, &contact->normal, contact->restitution);

    return contact;
}


void collisionObjectCollideTwoObjects(struct CollisionObject* a, struct CollisionObject* b, struct ContactSolver* contactSolver) {
    if (!box3DHasOverlap(&a->boundingBox, &b->boundingBox)) {
        return;
    }

    if (a->trigger && b->trigger) {
        return;
    }

    // Mesh collider can have multiple contacts, which it handles itself
    if (a->collider->type == CollisionShapeTypeMesh) {
        assert(a->collider->type != b->collider->type);
        meshColliderCollideObject(a, b, contactSolver);
        return;
    } else if (b->collider->type == CollisionShapeTypeMesh) {
        assert(a->collider->type != b->collider->type);
        meshColliderCollideObject(b, a, contactSolver);
        return;
    }

    // Compound colliders delegate to child collision
    if (a->collider->type == CollisionShapeTypeCompound) {
        compoundColliderCollideObject(a, b, contactSolver);
        return;
    } else if (b->collider->type == CollisionShapeTypeCompound) {
        compoundColliderCollideObject(b, a, contactSolver);
        return;
    }

    struct Simplex simplex;

    struct Vector3 offset;

    vector3Sub(&b->body->transform.position, &a->body->transform.position, &offset);

    if (!gjkCheckForOverlap(&simplex,
                a, objectMinkowskiSupport,
                b, objectMinkowskiSupport,
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
        a, objectMinkowskiSupport,
        b, objectMinkowskiSupport,
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

    // Mesh collider can have multiple contacts, which it handles itself
    if (a->collider->type == CollisionShapeTypeMesh) {
        assert(a->collider->type != b->collider->type);
        meshColliderCollideObject(a, b, contactSolver);
        return;
    } else if (b->collider->type == CollisionShapeTypeMesh) {
        assert(a->collider->type != b->collider->type);
        meshColliderCollideObject(b, a, contactSolver);
        return;
    }

    // Compound colliders delegate to child collision
    if (a->collider->type == CollisionShapeTypeCompound) {
        compoundColliderCollideObjectSwept(
            a, prevAPos, sweptA,
            b, prevBPos, sweptB,
            contactSolver
        );
        return;
    } else if (b->collider->type == CollisionShapeTypeCompound) {
        compoundColliderCollideObjectSwept(
            b, prevBPos, sweptB,
            a, prevAPos, sweptA,
            contactSolver
        );
        return;
    }

    struct Simplex simplex;

    struct Vector3 relativePrevPos;
    vector3Sub(prevBPos, prevAPos, &relativePrevPos);
    vector3Add(&relativePrevPos, &a->body->transform.position, &relativePrevPos);

    struct Vector3 offsetPrevBPos = *prevBPos;
    collisionObjectAddBodyOffset(b, &offsetPrevBPos);

    struct SweptCollisionObject sweptObject;
    sweptObject.object = b;
    sweptObject.prevPos = &offsetPrevBPos;

    if (!gjkCheckForOverlap(&simplex,
                a, objectMinkowskiSupport,
                &sweptObject, sweptObjectMinkowskiSupport,
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
        a, objectMinkowskiSupport,
        &sweptObject, sweptObjectMinkowskiSupport,
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

int quadMinkowskiSupport(void* data, struct Vector3* direction, struct Vector3* output) {
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

int sweptObjectMinkowskiSupport(void* data, struct Vector3* direction, struct Vector3* output) {
    struct SweptCollisionObject* sweptObject = (struct SweptCollisionObject*)data;
    struct ColliderTypeData* collider = sweptObject->object->collider;

    int result = collider->callbacks->minkowskiSupport(
        collider->data, 
        &sweptObject->object->body->rotationBasis,
        direction, 
        output
    );

    if (vector3Dot(sweptObject->object->position, direction) > vector3Dot(sweptObject->prevPos, direction)) {
        vector3Add(output, sweptObject->object->position, output);
    } else {
        vector3Add(output, sweptObject->prevPos, output);
    }

    return result;
}

int objectMinkowskiSupport(void* data, struct Vector3* direction, struct Vector3* output) {
    struct CollisionObject* object = (struct CollisionObject*)data;
    int result = object->collider->callbacks->minkowskiSupport(object->collider->data, &object->body->rotationBasis, direction, output);
    vector3Add(output, object->position, output);

    return result;
}

void collisionObjectLocalRay(struct CollisionObject* object, struct Ray* ray, struct Ray* localRay) {
    struct Vector3 offset;
    vector3Sub(&ray->origin, object->position, &offset);
    basisUnRotate(&object->body->rotationBasis, &ray->dir, &localRay->dir);
    basisUnRotate(&object->body->rotationBasis, &offset, &localRay->origin);
}

void collisionObjectAddBodyOffset(struct CollisionObject* object, struct Vector3* out) {
    if (object->position != &object->body->transform.position) {
        struct Vector3 bodyOffset;
        vector3Sub(object->position, &object->body->transform.position, &bodyOffset);
        vector3Add(out, &bodyOffset, out);
    }
}

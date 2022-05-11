#ifndef __CONTACT_SOLVER_H__
#define __CONTACT_SOLVER_H__

#include "../math/vector3.h"

struct CollisionObject;

struct VelocityState
{
	struct Vector3 w;
	struct Vector3 v;
};

struct ContactState
{
	int id;
	struct Vector3 ra;					// Vector from C.O.M to contact position
	struct Vector3 rb;					// Vector from C.O.M to contact position
	float penetration;			// Depth of penetration from collision
	float normalImpulse;			// Accumulated normal impulse
	float tangentImpulse[ 2 ];	// Accumulated friction impulse
	float bias;					// Restitution + baumgarte
	float normalMass;				// Normal constraint mass
	float tangentMass[ 2 ];		// Tangent constraint mass
};

#define MAX_CONTACTS_PER_MANIFOLD	8

#define NEGATIVE_PENETRATION_BIAS   0.00001f

struct ContactConstraintState
{
	struct ContactState contacts[ MAX_CONTACTS_PER_MANIFOLD ];
	int contactCount;
	struct Vector3 tangentVectors[ 2 ];	// Tangent vectors
	struct Vector3 normal;				// From A to B
	float restitution;
	float friction;
	struct CollisionObject* shapeA;
	struct CollisionObject* shapeB;
	struct ContactConstraintState* next;
};

#define MAX_CONTACT_COUNT	8

struct ContactSolver {
    struct ContactConstraintState contacts[MAX_CONTACT_COUNT];
	struct ContactConstraintState* unusedContacts;
	struct ContactConstraintState* activeContacts;
	int contactCapacity;
};

extern struct ContactSolver gContactSolver;

void contactSolverInit(struct ContactSolver* contactSolver);

void contactSolverSolve(struct ContactSolver* solver);

struct ContactConstraintState* contactSolverPeekContact(struct ContactSolver* solver, struct CollisionObject* shapeA, struct CollisionObject* shapeB);

void contactSolverRemoveContact(struct ContactSolver* solver, struct ContactConstraintState* toRemove);

struct ContactState* contactSolverGetContact(struct ContactConstraintState* contact, int id);

int contactSolverAssign(struct ContactConstraintState* into, struct ContactConstraintState* from, int filterPortalContacts);

#endif


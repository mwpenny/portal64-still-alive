#ifndef __CONTACT_SOLVER_H__
#define __CONTACT_SOLVER_H__

#include "../math/vector3.h"

struct CollisionObject;

struct VelocityState
{
	struct Vector3 w;
	struct Vector3 v;
};

struct ContactPoint
{
	int id;
	struct Vector3 contactALocal;					// Vector from C.O.M to contact position
	struct Vector3 contactBLocal;					// Vector from C.O.M to contact position
	struct Vector3 contactAWorld;					// Vector from C.O.M to contact position
	struct Vector3 contactBWorld;					// Vector from C.O.M to contact position
	float penetration;			// Depth of penetration from collision
	float normalImpulse;			// Accumulated normal impulse
	float tangentImpulse[ 2 ];	// Accumulated friction impulse
	float bias;					// Restitution + baumgarte
	float normalMass;				// Normal constraint mass
	float tangentMass[ 2 ];		// Tangent constraint mass
};

#define MAX_CONTACTS_PER_MANIFOLD	4

#define NEGATIVE_PENETRATION_BIAS   0.00001f

struct ContactManifold {
	struct ContactPoint contacts[ MAX_CONTACTS_PER_MANIFOLD ];
	short contactCount;
	struct Vector3 tangentVectors[ 2 ];	// Tangent vectors
	struct Vector3 normal;				// From A to B
	float restitution;
	float friction;
	struct CollisionObject* shapeA;
	struct CollisionObject* shapeB;
	struct ContactManifold* next;
};

#define MAX_CONTACT_COUNT	20

struct ContactSolver {
    struct ContactManifold contacts[MAX_CONTACT_COUNT];
	struct ContactManifold* unusedContacts;
	struct ContactManifold* activeContacts;
	int contactCapacity;
	short currentContactFrame;
};

extern struct ContactSolver gContactSolver;

void contactSolverInit(struct ContactSolver* contactSolver);

void contactSolverSolve(struct ContactSolver* solver);

struct ContactManifold* contactSolverGetContactManifold(struct ContactSolver* solver, struct CollisionObject* shapeA, struct CollisionObject* shapeB);

struct ContactManifold* contactSolverNextManifold(struct ContactSolver* solver, struct CollisionObject* forObject, struct ContactManifold* current);

void contactSolverRemoveUnusedContacts(struct ContactSolver* contactSolver);
void contactSolverCheckPortalContacts(struct ContactSolver* contactSolver, struct CollisionObject* objectWithNewPortal);
void contactSolverCleanupManifold(struct ContactManifold* manifold);

#endif


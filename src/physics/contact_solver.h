#ifndef __CONTACT_SOLVER_H__
#define __CONTACT_SOLVER_H__

#include "../math/vector3.h"

struct RigidBody;

#define Q3_BAUMGARTE 0.2f

#define Q3_PENETRATION_SLOP 0.05f

#define ENABLE_FRICTION 1

struct VelocityState
{
	struct Vector3 w;
	struct Vector3 v;
};

struct ContactState
{
	struct Vector3 ra;					// Vector from C.O.M to contact position
	struct Vector3 rb;					// Vector from C.O.M to contact position
	float penetration;			// Depth of penetration from collision
	float normalImpulse;			// Accumulated normal impulse
	float tangentImpulse[ 2 ];	// Accumulated friction impulse
	float bias;					// Restitution + baumgarte
	float normalMass;				// Normal constraint mass
	float tangentMass[ 2 ];		// Tangent constraint mass
};

struct ContactConstraintState
{
	struct ContactState contacts[ 8 ];
	int contactCount;
	struct Vector3 tangentVectors[ 2 ];	// Tangent vectors
	struct Vector3 normal;				// From A to B
	float restitution;
	float friction;
	struct RigidBody* bodyA;
	struct RigidBody* bodyB;
};

#define MAX_CONTACT_COUNT	8

struct ContactSolver {
    struct ContactConstraintState contacts[MAX_CONTACT_COUNT];
    int contactCount;
	int contactCapacity;
};

void contactSolverSolve(struct ContactSolver* solver);

#endif
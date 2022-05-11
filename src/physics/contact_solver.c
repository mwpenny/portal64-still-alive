
#include "contact_solver.h"
#include "../util/time.h"
#include "../math/mathf.h"
#include "rigid_body.h"
#include "collision_object.h"
#include "collision_scene.h"

#include <string.h>

#define Q3_BAUMGARTE 0.05f

#define Q3_PENETRATION_SLOP 0.001f

#define ENABLE_FRICTION 1

struct ContactSolver gContactSolver;

void contactSolverInit(struct ContactSolver* contactSolver) {
	memset(contactSolver, 0, sizeof(struct ContactSolver));

	contactSolver->contactCapacity = MAX_CONTACT_COUNT;

	contactSolver->unusedContacts = &contactSolver->contacts[0];

	for (int i = 1; i < MAX_CONTACT_COUNT; ++i) {
		contactSolver->contacts[i-1].next = &contactSolver->contacts[i];
	}
}

void contactSolverPreSolve(struct ContactSolver* contactSolver) {
	struct ContactConstraintState *cs = contactSolver->activeContacts;
	
    while (cs)
	{
		struct Vector3* vA;
		struct Vector3* wA;
		struct Vector3* vB;
		struct Vector3* wB;

		struct RigidBody* bodyA = cs->shapeA->body;
		struct RigidBody* bodyB = cs->shapeB->body;

		if (bodyA) {
			vA = &bodyA->velocity;
			wA = &bodyA->angularVelocity;
		} else {
			vA = NULL;
			wA = NULL;
		}

		if (bodyB) {
			vB = &bodyB->velocity;
			wB = &bodyB->angularVelocity;
		} else {
			vB = NULL;
			wB = NULL;
		}

		for ( int j = 0; j < cs->contactCount; ++j )
		{
			struct ContactState *c = cs->contacts + j;

			// Precalculate JM^-1JT for contact and friction constraints
			struct Vector3 raCn;
            vector3Cross(&c->ra, &cs->normal, &raCn);
			struct Vector3 rbCn;
            vector3Cross(&c->rb, &cs->normal, &rbCn);
			float nm = 0;
			
			if (bodyA) {
				nm += bodyA->massInv;
			}

			if (bodyB) {
				nm += bodyB->massInv;
			}

			float tm[ 2 ];
			tm[ 0 ] = nm;
			tm[ 1 ] = nm;

			if (bodyA) {
				nm += bodyA->momentOfInertiaInv * vector3MagSqrd(&raCn);
			}

			if (bodyB) {
				nm += bodyB->momentOfInertiaInv * vector3MagSqrd(&rbCn);
			}

			c->normalMass = safeInvert( nm );

			for (int i = 0; i < 2; ++i )
			{
				struct Vector3 raCt;
                vector3Cross(&cs->tangentVectors[ i ], &c->ra, &raCt);
				struct Vector3 rbCt;
                vector3Cross(&cs->tangentVectors[ i ], &c->rb, &rbCt);

				if (bodyA) {
					tm[ i ] += bodyA->momentOfInertiaInv * vector3MagSqrd(&raCt);
				}
				if (bodyB) {
					tm[ i ] += bodyB->momentOfInertiaInv * vector3MagSqrd(&rbCt);
				}

				c->tangentMass[ i ] = safeInvert( tm[ i ] );
			}

			// Precalculate bias factor
			c->bias = -Q3_BAUMGARTE * (1.0f / FIXED_DELTA_TIME) * minf(0.0f, c->penetration + Q3_PENETRATION_SLOP);
			
			// Warm start contact
			struct Vector3 P;
            vector3Scale(&cs->normal, &P, c->normalImpulse);

			if ( ENABLE_FRICTION )
			{
                vector3AddScaled(&P, &cs->tangentVectors[0], c->tangentImpulse[0], &P);
                vector3AddScaled(&P, &cs->tangentVectors[1], c->tangentImpulse[1], &P);
			}

            struct Vector3 w;

			if (bodyA) {
				vector3AddScaled(vA, &P, -bodyA->massInv, vA);
				vector3Cross(&c->ra, &P, &w);
				vector3AddScaled(wA, &w, -bodyA->momentOfInertiaInv, wA);
			}

			if (bodyB) {
				vector3AddScaled(vB, &P, bodyB->massInv, vB);
				vector3Cross(&c->rb, &P, &w);
				vector3AddScaled(wB, &w, bodyB->momentOfInertiaInv, wB);
			}

            struct Vector3 velocity;
            struct Vector3 angularVelocity;

			if (vB) {
				vector3Cross(wB, &c->rb, &angularVelocity);
				vector3Add(&angularVelocity, vB, &velocity);
			} else {
				velocity = gZeroVec;
			}

			if (vA) {
				vector3Cross(wA, &c->ra, &angularVelocity);
				vector3Sub(&velocity, &angularVelocity, &velocity);
				vector3Sub(&velocity, vA, &velocity);
			}

            float dv = vector3Dot(&velocity, &cs->normal);

			if ( dv < -1.0f )
				c->bias += -(cs->restitution) * dv;
		}

		cs = cs->next;
	}
}

void contactSolverIterate(struct ContactSolver* contactSolver) {
	struct ContactConstraintState *cs = contactSolver->activeContacts;

    while (cs)
	{
		struct Vector3* vA;
		struct Vector3* wA;
		struct Vector3* vB;
		struct Vector3* wB;

		struct RigidBody* bodyA = cs->shapeA->body;
		struct RigidBody* bodyB = cs->shapeB->body;

		if (bodyA) {
			vA = &bodyA->velocity;
			wA = &bodyA->angularVelocity;
		} else {
			vA = NULL;
			wA = NULL;
		}

		if (bodyB) {
			vB = &bodyB->velocity;
			wB = &bodyB->angularVelocity;
		} else {
			vB = NULL;
			wB = NULL;
		}

		for ( int j = 0; j < cs->contactCount; ++j )
		{
			struct ContactState *c = cs->contacts + j;

			// relative velocity at contact
            struct Vector3 dv;
            struct Vector3 angularVelocity;

			if (wB) {
				vector3Cross(wB, &c->rb, &angularVelocity);
				vector3Add(&angularVelocity, vB, &dv);
			} else {
				dv = gZeroVec;
			}

			if (wA) {
				vector3Cross(wA, &c->ra, &angularVelocity);
				vector3Sub(&dv, &angularVelocity, &dv);
				vector3Sub(&dv, vA, &dv);
			}

			// Friction
			if ( ENABLE_FRICTION )
			{
				for ( int i = 0; i < 2; ++i )
				{
					float lambda = -vector3Dot( &dv, &cs->tangentVectors[ i ] ) * c->tangentMass[ i ];

					// Calculate frictional impulse
					float maxLambda = cs->friction * c->normalImpulse;

					// Clamp frictional impulse
					float oldPT = c->tangentImpulse[ i ];
					c->tangentImpulse[ i ] = clampf( oldPT + lambda , -maxLambda, maxLambda);
					lambda = c->tangentImpulse[ i ] - oldPT;

					// Apply friction impulse
					struct Vector3 impulse;
                    vector3Scale(&cs->tangentVectors[i], &impulse, lambda);

                    struct Vector3 w;
					if (vA) {
						vector3AddScaled(vA, &impulse, -bodyA->massInv, vA);
						vector3Cross(&c->ra, &impulse, &w);
						vector3AddScaled(wA, &w, -bodyA->momentOfInertiaInv, wA);
					}

					if (vB) {
						vector3AddScaled(vB, &impulse, bodyB->massInv, vB);
						vector3Cross(&c->rb, &impulse, &w);
						vector3AddScaled(wB, &w, bodyB->momentOfInertiaInv, wB);
					}
				}
			}

			// Normal
			{
				if (wB) {
					vector3Cross(wB, &c->rb, &angularVelocity);
					vector3Add(&angularVelocity, vB, &dv);
				} else {
					dv = gZeroVec;
				}

				if (wA) {
					vector3Cross(wA, &c->ra, &angularVelocity);
					vector3Sub(&dv, &angularVelocity, &dv);
					vector3Sub(&dv, vA, &dv);
				}

				// Normal impulse
				float vn = vector3Dot( &dv, &cs->normal );

				// Factor in positional bias to calculate impulse scalar j
				float lambda = c->normalMass * (-vn + c->bias);

				// Clamp impulse
				float tempPN = c->normalImpulse;
				c->normalImpulse = maxf( tempPN + lambda, 0.0f );
				lambda = c->normalImpulse - tempPN;

				// Apply impulse
				struct Vector3 impulse;
                vector3Scale(&cs->normal, &impulse, lambda);

                struct Vector3 w;
				if (vA) {
					vector3AddScaled(vA, &impulse, -bodyA->massInv, vA);
					vector3Cross(&c->ra, &impulse, &w);
					vector3AddScaled(wA, &w, -bodyA->momentOfInertiaInv, wA);
				}

				if (vB) {
					vector3AddScaled(vB, &impulse, bodyB->massInv, vB);
					vector3Cross(&c->rb, &impulse, &w);
					vector3AddScaled(wB, &w, bodyB->momentOfInertiaInv, wB);
				}
			}
		}

		cs = cs->next;
    }
}


void contactSolverSolve(struct ContactSolver* solver) {
	contactSolverPreSolve(solver);
	contactSolverIterate(solver);
	contactSolverIterate(solver);
	// contactSolverIterate(solver);
}

struct ContactConstraintState* contactSolverPeekContact(struct ContactSolver* solver, struct CollisionObject* shapeA, struct CollisionObject* shapeB) {
	struct ContactConstraintState* curr = solver->activeContacts;

	while (curr) {
		if ((curr->shapeA == shapeA && curr->shapeB == shapeB) || (curr->shapeA == shapeB && curr->shapeB == shapeA)) {
			return curr;
		}

		curr = curr->next;
	}

	struct ContactConstraintState* result = solver->unusedContacts;

	if (result) {
		result->shapeA = shapeA;
		result->shapeB = shapeB;
		result->contactCount = 0;

		solver->unusedContacts = result->next;
		result->next = solver->activeContacts;
		solver->activeContacts = result;
	}

	return result;
}

void contactSolverRemoveContact(struct ContactSolver* solver, struct ContactConstraintState* toRemove) {
	struct ContactConstraintState* curr = solver->activeContacts;
	struct ContactConstraintState* prev = NULL;

	while (curr) {
		if (curr == toRemove) {
			break;
		}

		prev = curr;
		curr = curr->next;
	}

	if (!curr) {
		return;
	}

	if (prev) {
		prev->next = curr->next;
	} else {
		solver->activeContacts = curr->next;
	}

	curr->next = solver->unusedContacts;
	solver->unusedContacts = curr;
}

struct ContactState* contactSolverGetContact(struct ContactConstraintState* contact, int id) {
	int i;

	for (i = 0; i < contact->contactCount; ++i) {
		if (contact->contacts[i].id == id) {
			return &contact->contacts[i];
		}
	}

	if (i == MAX_CONTACTS_PER_MANIFOLD) {
		return NULL;
	}

	struct ContactState* result = &contact->contacts[i];

	result->normalImpulse = 0.0f;
	result->tangentImpulse[0] = 0.0f;
	result->tangentImpulse[1] = 0.0f;
	result->id = id;

	++contact->contactCount;
	
	return result;
}

int contactSolverAssign(struct ContactConstraintState* into, struct ContactConstraintState* from, int filterPortalContacts) {
	for (int sourceIndex = 0; sourceIndex < from->contactCount; ++sourceIndex) {
		int targetIndex;

		struct ContactState* sourceContact = &from->contacts[sourceIndex];

		for (targetIndex = 0; targetIndex < into->contactCount; ++targetIndex) {
			struct ContactState* targetContact = &into->contacts[targetIndex];

			if (sourceContact->id == targetContact->id) {
				sourceContact->normalImpulse = targetContact->normalImpulse;
				// TODO reproject tangents
				sourceContact->tangentImpulse[0] = targetContact->tangentImpulse[0];
				sourceContact->tangentImpulse[1] = targetContact->tangentImpulse[1];
				break;
			}
		}
	}

	int copiedCount = 0;
	int result = 0;

	for (int sourceIndex = 0; sourceIndex < from->contactCount; ++sourceIndex) {
		if (filterPortalContacts && collisionSceneIsTouchingPortal(&from->contacts[sourceIndex].ra)) {
			result = 1;
			continue;
		}

		into->contacts[copiedCount] = from->contacts[sourceIndex];
		++copiedCount;
	}

	into->contactCount = copiedCount;
	into->tangentVectors[0] = from->tangentVectors[0];
	into->tangentVectors[1] = from->tangentVectors[1];
	into->normal = from->normal;
	into->restitution = from->restitution;
	into->friction = from->friction;
	
	return result;
}
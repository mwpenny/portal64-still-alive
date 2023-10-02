
#include "contact_solver.h"
#include "../util/time.h"
#include "../math/mathf.h"
#include "rigid_body.h"
#include "collision_object.h"
#include "collision_scene.h"

#include <string.h>

#define Q3_BAUMGARTE 0.15f

#define Q3_PENETRATION_SLOP 0.001f

#define SLIDE_TOLERANCE	0.1f

#define SEPERATION_TOLERANCE	0.2f

#define ENABLE_FRICTION 1

#define SOLVER_ITERATIONS	8

struct ContactSolver gContactSolver;

void contactSolverCleanupManifold(struct ContactManifold* manifold) {
	int writeIndex = 0;

	for (int readIndex = 0; readIndex < manifold->contactCount; ++readIndex) {
		struct ContactPoint* contactPoint = &manifold->contacts[readIndex];
		struct Vector3 offset;

		struct Vector3 worldPosA;
		struct Vector3 worldPosB;

		if (manifold->shapeA->body) {
			transformPoint(&manifold->shapeA->body->transform, &contactPoint->contactALocal, &worldPosA);
		} else {
			worldPosA = contactPoint->contactALocal;
		}

		if (manifold->shapeB->body) {
			transformPoint(&manifold->shapeB->body->transform, &contactPoint->contactBLocal, &worldPosB);
		} else {
			worldPosB = contactPoint->contactBLocal;
		}

		vector3Sub(&worldPosB, &worldPosA, &offset);

		contactPoint->penetration = vector3Dot(&offset, &manifold->normal);

		// skip this point to remove it
		if (fabsf(contactPoint->penetration) > SEPERATION_TOLERANCE) {
			continue;
		}

		vector3AddScaled(&offset, &manifold->normal, -contactPoint->penetration, &offset);

		if (vector3MagSqrd(&offset) > SLIDE_TOLERANCE * SLIDE_TOLERANCE) {
			continue;
		}

		// update the world radius for the contact solver
		if (manifold->shapeA->body) {
			vector3Sub(&worldPosA, &manifold->shapeA->body->transform.position, &contactPoint->contactAWorld);
		}

		if (manifold->shapeB->body) {
			vector3Sub(&worldPosB, &manifold->shapeB->body->transform.position, &contactPoint->contactBWorld);
		}

		if (readIndex != writeIndex) {
			manifold->contacts[writeIndex] = *contactPoint;
		}

		++writeIndex;
	}

	manifold->contactCount = writeIndex;
}

void contactSolverManifoldCleanup(struct ContactSolver* contactSolver, struct ContactManifold* contact) {
	int idMask = ~(1 << (contact - contactSolver->contacts));
	contact->shapeA->manifoldIds &= idMask;
	contact->shapeB->manifoldIds &= idMask;

	if (contact->shapeA->body) {
		contact->shapeA->body->flags &= ~RigidBodyIsSleeping;
	}

	if (contact->shapeB->body) {
		contact->shapeB->body->flags &= ~RigidBodyIsSleeping;
	}
}

void contactSolverRemoveUnusedContacts(struct ContactSolver* contactSolver) {
	struct ContactManifold* curr = contactSolver->activeContacts;
	struct ContactManifold* prev = NULL;

	while (curr) {
		contactSolverCleanupManifold(curr);

		if (curr->contactCount == 0) {
			contactSolverManifoldCleanup(contactSolver, curr);

			if (prev) {
				prev->next = curr->next;
			} else {
				contactSolver->activeContacts = curr->next;
			}

			struct ContactManifold* next = curr->next;
			curr->next = contactSolver->unusedContacts;
			contactSolver->unusedContacts = curr;
			curr = next;
		} else {
			curr->shapeA->flags |= COLLISION_OBJECT_HAS_CONTACTS;
			curr->shapeB->flags |= COLLISION_OBJECT_HAS_CONTACTS;

			prev = curr;
			curr = curr->next;
		}
	}
}

void contactSolverCheckPortalManifoldContacts(struct ContactManifold* manifold) {
	int writeIndex = 0;

	for (int readIndex = 0; readIndex < manifold->contactCount; ++readIndex) {
		struct ContactPoint* contactPoint = &manifold->contacts[readIndex];

		if (collisionSceneIsTouchingPortal(&contactPoint->contactAWorld, &manifold->normal)) {
			continue;
		}

		if (readIndex != writeIndex) {
			manifold->contacts[writeIndex] = *contactPoint;
		}

		++writeIndex;
	}

	if (writeIndex != manifold->contactCount) {
		manifold->contactCount = writeIndex;
		manifold->shapeB->body->flags &= ~RigidBodyIsSleeping;
	}
}

void contactSolverCheckPortalContacts(struct ContactSolver* contactSolver) {	
	struct ContactManifold* curr = contactSolver->activeContacts;
	struct ContactManifold* prev = NULL;

	while (curr) {
		if (curr->shapeA->body == NULL) {
			contactSolverCheckPortalManifoldContacts(curr);
		}

		if (curr->contactCount == 0) {
			contactSolverManifoldCleanup(contactSolver, curr);
			if (prev) {
				prev->next = curr->next;
			} else {
				contactSolver->activeContacts = curr->next;
			}

			struct ContactManifold* next = curr->next;
			curr->next = contactSolver->unusedContacts;
			contactSolver->unusedContacts = curr;
			curr = next;
		} else {
			prev = curr;
			curr = curr->next;
		}
	}
}

void contactSolverInit(struct ContactSolver* contactSolver) {
	int solverSize = sizeof(struct ContactSolver);
	memset(contactSolver, 0, solverSize);

	contactSolver->contactCapacity = MAX_CONTACT_COUNT;

	contactSolver->unusedContacts = &contactSolver->contacts[0];

	for (int i = 1; i < MAX_CONTACT_COUNT; ++i) {
		contactSolver->contacts[i-1].next = &contactSolver->contacts[i];
	}

	contactSolver->firstPointConstraint = NULL;
}

void contactSolverPreSolve(struct ContactSolver* contactSolver) {
	struct ContactManifold *cs = contactSolver->activeContacts;
	
    while (cs)
	{
		if (!collisionObjectIsActive(cs->shapeA) && !collisionObjectIsActive(cs->shapeB)) {
			cs = cs->next;
			continue;
		}

		struct Vector3* vA;
		struct Vector3* wA;
		struct Vector3* vB;
		struct Vector3* wB;

		struct RigidBody* bodyA = cs->shapeA->body;
		struct RigidBody* bodyB = cs->shapeB->body;

		if (bodyA && !(bodyA->flags & RigidBodyIsKinematic)) {
			vA = &bodyA->velocity;
			wA = &bodyA->angularVelocity;
		} else {
			vA = NULL;
			wA = NULL;
		}

		if (bodyB && !(bodyB->flags & RigidBodyIsKinematic)) {
			vB = &bodyB->velocity;
			wB = &bodyB->angularVelocity;
		} else {
			vB = NULL;
			wB = NULL;
		}

		for ( int j = 0; j < cs->contactCount; ++j )
		{
			struct ContactPoint *c = cs->contacts + j;

			// Precalculate JM^-1JT for contact and friction constraints
			struct Vector3 raCn;
            vector3Cross(&c->contactAWorld, &cs->normal, &raCn);
			struct Vector3 rbCn;
            vector3Cross(&c->contactBWorld, &cs->normal, &rbCn);
			float nm = 0;
			
			if (vA) {
				nm += bodyA->massInv;
			}

			if (vB) {
				nm += bodyB->massInv;
			}

			float tm[ 2 ];
			tm[ 0 ] = nm;
			tm[ 1 ] = nm;

			if (vA) {
				nm += bodyA->momentOfInertiaInv * vector3MagSqrd(&raCn);
			}

			if (vB) {
				nm += bodyB->momentOfInertiaInv * vector3MagSqrd(&rbCn);
			}

			c->normalMass = safeInvert( nm );

			for (int i = 0; i < 2; ++i )
			{
				struct Vector3 raCt;
                vector3Cross(&cs->tangentVectors[ i ], &c->contactAWorld, &raCt);
				struct Vector3 rbCt;
                vector3Cross(&cs->tangentVectors[ i ], &c->contactBWorld, &rbCt);

				if (vA) {
					tm[ i ] += bodyA->momentOfInertiaInv * vector3MagSqrd(&raCt);
				}
				if (vB) {
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

			if (vA) {
				vector3AddScaled(vA, &P, -bodyA->massInv, vA);
				vector3Cross(&c->contactAWorld, &P, &w);
				vector3AddScaled(wA, &w, -bodyA->momentOfInertiaInv, wA);
			}

			if (vB) {
				vector3AddScaled(vB, &P, bodyB->massInv, vB);
				vector3Cross(&c->contactBWorld, &P, &w);
				vector3AddScaled(wB, &w, bodyB->momentOfInertiaInv, wB);
			}

            struct Vector3 velocity;
            struct Vector3 angularVelocity;

			if (vB) {
				vector3Cross(wB, &c->contactBWorld, &angularVelocity);
				vector3Add(&angularVelocity, vB, &velocity);
			} else {
				velocity = gZeroVec;
			}

			if (vA) {
				vector3Cross(wA, &c->contactAWorld, &angularVelocity);
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

#define BREAK_DISTANCE	0.5f

void contactSolverIterateConstraints(struct ContactSolver* contactSolver) {
	struct PointConstraint* curr = contactSolver->firstPointConstraint;

	struct PointConstraint* prev = NULL;

	while (curr) {
		if (!pointConstraintMoveToPoint(curr->object, &curr->targetPos, curr->maxPosImpulse, curr->movementScaleFactor)) {
			struct PointConstraint* next = curr->nextConstraint;

			if (prev) {
				prev->nextConstraint = next;
			} else {
				contactSolver->firstPointConstraint = next;
			}


			curr->nextConstraint = NULL;
			curr->object = NULL;

			curr = next;
			continue;
		}

		pointConstraintRotateTo(curr->object->body, &curr->targetRot, curr->maxRotImpulse);		

		curr = curr->nextConstraint;
	} 
}

void contactSolverIterate(struct ContactSolver* contactSolver) {
	struct ContactManifold *cs = contactSolver->activeContacts;

    while (cs)
	{
		if (!collisionObjectIsActive(cs->shapeA) && !collisionObjectIsActive(cs->shapeB)) {
			cs = cs->next;
			continue;
		}

		struct Vector3* vA;
		struct Vector3* wA;
		struct Vector3* vB;
		struct Vector3* wB;

		struct RigidBody* bodyA = cs->shapeA->body;
		struct RigidBody* bodyB = cs->shapeB->body;

		if (bodyA && !(bodyA->flags & RigidBodyIsKinematic)) {
			vA = &bodyA->velocity;
			wA = &bodyA->angularVelocity;
		} else {
			vA = NULL;
			wA = NULL;
		}

		if (bodyB && !(bodyB->flags & RigidBodyIsKinematic)) {
			vB = &bodyB->velocity;
			wB = &bodyB->angularVelocity;
		} else {
			vB = NULL;
			wB = NULL;
		}

		for ( int j = 0; j < cs->contactCount; ++j )
		{
			struct ContactPoint *c = cs->contacts + j;

			// relative velocity at contact
            struct Vector3 dv;
            struct Vector3 angularVelocity;

			if (wB) {
				vector3Cross(wB, &c->contactBWorld, &angularVelocity);
				vector3Add(&angularVelocity, vB, &dv);
			} else {
				dv = gZeroVec;
			}

			if (wA) {
				vector3Cross(wA, &c->contactAWorld, &angularVelocity);
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
						vector3Cross(&c->contactAWorld, &impulse, &w);
						vector3AddScaled(wA, &w, -bodyA->momentOfInertiaInv, wA);
					}

					if (vB) {
						vector3AddScaled(vB, &impulse, bodyB->massInv, vB);
						vector3Cross(&c->contactBWorld, &impulse, &w);
						vector3AddScaled(wB, &w, bodyB->momentOfInertiaInv, wB);
					}
				}
			}

			// Normal
			{
				if (wB) {
					vector3Cross(wB, &c->contactBWorld, &angularVelocity);
					vector3Add(&angularVelocity, vB, &dv);
				} else {
					dv = gZeroVec;
				}

				if (wA) {
					vector3Cross(wA, &c->contactAWorld, &angularVelocity);
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
					vector3Cross(&c->contactAWorld, &impulse, &w);
					vector3AddScaled(wA, &w, -bodyA->momentOfInertiaInv, wA);
				}

				if (vB) {
					vector3AddScaled(vB, &impulse, bodyB->massInv, vB);
					vector3Cross(&c->contactBWorld, &impulse, &w);
					vector3AddScaled(wB, &w, bodyB->momentOfInertiaInv, wB);
				}
			}
		}

		cs = cs->next;
    }
}


void contactSolverSolve(struct ContactSolver* solver) {
	contactSolverIterateConstraints(solver);
	contactSolverPreSolve(solver);
	for (int i = 0; i < SOLVER_ITERATIONS; ++i) {
		contactSolverIterate(solver);
	}
}

struct ContactManifold* contactSolverGetContactManifold(struct ContactSolver* solver, struct CollisionObject* shapeA, struct CollisionObject* shapeB) {
	struct ContactManifold* curr = solver->activeContacts;

	while (curr) {
		if ((curr->shapeA == shapeA && curr->shapeB == shapeB) || (curr->shapeA == shapeB && curr->shapeB == shapeA)) {
			return curr;
		}

		curr = curr->next;
	}

	struct ContactManifold* result = solver->unusedContacts;

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

struct ContactManifold* contactSolverNextManifold(struct ContactSolver* solver, struct CollisionObject* forObject, struct ContactManifold* current) {
	if (!current) {
		current = solver->activeContacts;
	} else {
		current = current->next;
	}

	while (current) {
		if (current->shapeA == forObject || current->shapeB == forObject) {
			return current;
		}

		current = current->next;
	}

	return NULL;
}


float contactPenetration(struct ContactManifold* contact) {
	float result = 0;

	for (int i = 0; i < contact->contactCount; ++i) {
		if (contact->contacts[i].penetration > result) {
			result = contact->contacts[i].penetration;
		}
	}

	return result;
}

void contactAdjustPenetration(struct ContactManifold* contact, float amount) {
	for (int i = 0; i < contact->contactCount; ++i) {
		contact->contacts[i].penetration += amount;
	}
}

void contactSolverAddPointConstraint(struct ContactSolver* solver, struct PointConstraint* constraint) {
    constraint->nextConstraint = solver->firstPointConstraint;
    solver->firstPointConstraint = constraint;
}

void contactSolverRemovePointConstraint(struct ContactSolver* solver, struct PointConstraint* constraint) {
    struct PointConstraint* prev = NULL;
    struct PointConstraint* current = solver->firstPointConstraint;

    while (current) {
        if (current == constraint) {
            if (prev) {
                prev->nextConstraint = current->nextConstraint;
            } else {
                solver->firstPointConstraint = current->nextConstraint;
            }

            return;
        }
        
        prev = current;
        current = current->nextConstraint;
    }
}

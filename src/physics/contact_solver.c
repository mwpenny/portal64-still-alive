
#include "contact_solver.h"
#include "../util/time.h"
#include "../math/mathf.h"
#include "rigid_body.h"

void contactSolverPreSolve(struct ContactSolver* contactSolver) {
	for ( int i = 0; i < contactSolver->contactCount; ++i )
	{
		struct ContactConstraintState *cs = contactSolver->contacts + i;

		struct Vector3* vA;
		struct Vector3* wA;
		struct Vector3* vB;
		struct Vector3* wB;

		if (cs->bodyA) {
			vA = &cs->bodyA->velocity;
			wA = &cs->bodyA->angularVelocity;
		} else {
			vA = NULL;
			wA = NULL;
		}

		if (cs->bodyB) {
			vB = &cs->bodyB->velocity;
			wB = &cs->bodyB->angularVelocity;
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
			
			if (cs->bodyA) {
				nm += cs->bodyA->massInv;
			}

			if (cs->bodyB) {
				nm += cs->bodyB->massInv;
			}

			float tm[ 2 ];
			tm[ 0 ] = nm;
			tm[ 1 ] = nm;

			if (cs->bodyA) {
				nm += cs->bodyA->momentOfInertiaInv * vector3MagSqrd(&raCn);
			}

			if (cs->bodyB) {
				nm += cs->bodyB->momentOfInertiaInv * vector3MagSqrd(&rbCn);
			}

			c->normalMass = safeInvert( nm );

			for (int i = 0; i < 2; ++i )
			{
				struct Vector3 raCt;
                vector3Cross(&cs->tangentVectors[ i ], &c->ra, &raCt);
				struct Vector3 rbCt;
                vector3Cross(&cs->tangentVectors[ i ], &c->rb, &rbCt);

				if (cs->bodyA) {
					tm[ i ] += cs->bodyA->momentOfInertiaInv * vector3MagSqrd(&raCt);
				}
				if (cs->bodyB) {
					tm[ i ] += cs->bodyB->momentOfInertiaInv * vector3MagSqrd(&rbCt);
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

			if (cs->bodyA) {
				vector3AddScaled(vA, &P, -cs->bodyA->massInv, vA);
				vector3Cross(&c->ra, &P, &w);
				vector3AddScaled(wA, &w, -cs->bodyA->momentOfInertiaInv, wA);
			}

			if (cs->bodyB) {
				vector3AddScaled(vB, &P, cs->bodyB->massInv, vB);
				vector3Cross(&c->rb, &P, &w);
				vector3AddScaled(wB, &w, cs->bodyB->momentOfInertiaInv, wB);
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
	}
}

void contactSolverIterate(struct ContactSolver* contactSolver) {
    for ( int i = 0; i < contactSolver->contactCount; ++i )
	{
		struct ContactConstraintState *cs = contactSolver->contacts + i;

		struct Vector3* vA;
		struct Vector3* wA;
		struct Vector3* vB;
		struct Vector3* wB;

		if (cs->bodyA) {
			vA = &cs->bodyA->velocity;
			wA = &cs->bodyA->angularVelocity;
		} else {
			vA = NULL;
			wA = NULL;
		}

		if (cs->bodyB) {
			vB = &cs->bodyB->velocity;
			wB = &cs->bodyB->angularVelocity;
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
						vector3AddScaled(vA, &impulse, -cs->bodyA->massInv, vA);
						vector3Cross(&c->ra, &impulse, &w);
						vector3AddScaled(wA, &w, -cs->bodyA->momentOfInertiaInv, wA);
					}

					if (vB) {
						vector3AddScaled(vB, &impulse, cs->bodyB->massInv, vB);
						vector3Cross(&c->rb, &impulse, &w);
						vector3AddScaled(wB, &w, cs->bodyB->momentOfInertiaInv, wB);
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
					vector3AddScaled(vA, &impulse, -cs->bodyA->massInv, vA);
					vector3Cross(&c->ra, &impulse, &w);
					vector3AddScaled(wA, &w, -cs->bodyA->momentOfInertiaInv, wA);
				}

				if (vB) {
					vector3AddScaled(vB, &impulse, cs->bodyB->massInv, vB);
					vector3Cross(&c->rb, &impulse, &w);
					vector3AddScaled(wB, &w, cs->bodyB->momentOfInertiaInv, wB);
				}
			}
		}
    }
}


void contactSolverSolve(struct ContactSolver* solver) {
	contactSolverPreSolve(solver);
	contactSolverIterate(solver);
	contactSolverIterate(solver);
	// contactSolverIterate(solver);
}
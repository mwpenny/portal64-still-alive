#include "gjk.h"

void simplexInit(struct Simplex* simplex, struct Vector3* firstPoint) {
    simplex->nPoints = 1;
    simplex->points[0] = *firstPoint;
}

int simplexAddPoint(struct Simplex* simplex, struct Vector3* point) {
    if (simplex->nPoints == MAX_SIMPLEX_SIZE) {
        // SHOULD never happen, but just in case
        return 0;
    }

    simplex->points[simplex->nPoints] = *point;
    ++simplex->nPoints;

    return 1;
}

int simplexCheck(struct Simplex* simplex, struct Vector3* nextDirection) {
    struct Vector3* lastAdded = &simplex->points[simplex->nPoints - 1];
    struct Vector3 aToOrigin;
    vector3Negate(lastAdded, &aToOrigin);

    if (simplex->nPoints == 2) {
        struct Vector3 lastAddedToOther;
        vector3Sub(&simplex->points[0], lastAdded, &lastAddedToOther);
        vector3TripleProduct(&lastAddedToOther, &aToOrigin, &lastAddedToOther, nextDirection);

        if (vector3MagSqrd(nextDirection) <= 0.0000001f) {
            vector3Perp(&lastAddedToOther, nextDirection);
        }

        return 0;
    } else if (simplex->nPoints == 3) {
        struct Vector3 normal;
        struct Vector3 ab;
        vector3Sub(&simplex->points[1], lastAdded, &ab);
        struct Vector3 ac;
        vector3Sub(&simplex->points[0], lastAdded, &ac);

        vector3Cross(&ab, &ac, &normal);

        struct Vector3 dirCheck;
        vector3Cross(&ab, &normal, &dirCheck);

        if (vector3Dot(&dirCheck, &aToOrigin) > 0.0f) {
            vector3TripleProduct(&ab, &aToOrigin, &ab, nextDirection);

            if (vector3MagSqrd(nextDirection) <= 0.0000001f) {
                *nextDirection = normal;
            }

            // remove c
            simplex->points[0] = simplex->points[1];
            simplex->points[1] = simplex->points[2];
            simplex->nPoints = 2;

            return 0;
        }

        vector3Cross(&normal, &ac, &dirCheck);

        if (vector3Dot(&dirCheck, &aToOrigin) > 0.0f) {
            vector3TripleProduct(&ac, &aToOrigin, &ac, nextDirection);

            if (vector3MagSqrd(nextDirection) <= 0.0000001f) {
                *nextDirection = normal;
            }

            // remove b
            simplex->points[1] = simplex->points[2];
            simplex->nPoints = 2;

            return 0;
        }

        if (vector3Dot(&normal, &aToOrigin) > 0.0f) {
            *nextDirection = normal;
            return 0;
        }

        // change triangle winding
        struct Vector3 tmp = simplex->points[0];
        simplex->points[0] = simplex->points[1];
        simplex->points[1] = tmp;
        vector3Negate(&normal, nextDirection);
    } else if (simplex->nPoints == 4) {
        int lastBehindIndex = -1;
        int lastInFrontIndex = -1;
        int isFrontCount = 0;

        struct Vector3 normals[3];

        for (int i = 0; i < 3; ++i) {
            struct Vector3 firstEdge;
            struct Vector3 secondEdge;
            vector3Sub(lastAdded, &simplex->points[i], &firstEdge);
            vector3Sub(i == 2 ? &simplex->points[0] : &simplex->points[i + 1], &simplex->points[i], &secondEdge);
            vector3Cross(&firstEdge, &secondEdge, &normals[i]);

            if (vector3Dot(&aToOrigin, &normals[i]) > 0.0f) {
                ++isFrontCount;
                lastInFrontIndex = i;
            } else {
                lastBehindIndex = i;
            }
        }

        if (isFrontCount == 0) {
            // enclosed the origin
            return 1;
        } else if (isFrontCount == 1) {
            *nextDirection = normals[lastInFrontIndex];

            if (lastInFrontIndex == 1) {
                simplex->points[0] = simplex->points[1];
                simplex->points[1] = simplex->points[2];
            } else if (lastInFrontIndex == 2) {
                simplex->points[1] = simplex->points[0];
                simplex->points[0] = simplex->points[2];
            }

            simplex->points[2] = simplex->points[3];
            simplex->nPoints = 3;
        } else if (isFrontCount == 2) {
            if (lastBehindIndex == 0) {
                simplex->points[0] = simplex->points[2];
            } else if (lastBehindIndex == 2) {
                simplex->points[0] = simplex->points[1];
            }

            simplex->points[1] = simplex->points[3];
            simplex->nPoints = 2;

            struct Vector3 ab;
            vector3Sub(&simplex->points[0], &simplex->points[1], &ab);

            vector3TripleProduct(&ab, &aToOrigin, &ab, nextDirection);

            if (vector3MagSqrd(nextDirection) <= 0.0000001f) {
                vector3Perp(&ab, nextDirection);
            }
        } else {
            // this case shouldn't happen but if it does
            // this is the correct logic
            simplex->points[0] = simplex->points[3];
            simplex->nPoints = 1;
            *nextDirection = aToOrigin;
        }
    }

    return 0;
}

#define MAX_GJK_ITERATIONS  10

int gjkCheckForOverlap(struct Simplex* simplex, void* data, MinkowsiSum sum, struct Vector3* firstDirection) {
    struct Vector3 nextPoint;
    struct Vector3 nextDirection;

    if (vector3IsZero(firstDirection)) {
        sum(data, &gRight, &nextPoint);
        vector3Negate(&gRight, &nextDirection);
    } else {
        sum(data, firstDirection, &nextPoint);
        vector3Negate(firstDirection, &nextDirection);
    }

    simplexInit(simplex, &nextPoint);

    for (int iteration = 0; iteration < MAX_GJK_ITERATIONS; ++iteration) {
        sum(data, &nextDirection, &nextPoint);
        
        if (vector3Dot(&nextPoint, &nextDirection) <= 0.0f) {
            return 0;
        }

        if (!simplexAddPoint(simplex, &nextPoint)) {
            return 0;
        }

        if (simplexCheck(simplex, &nextDirection)) {
            return 1;
        }

    }

    return 0;
}
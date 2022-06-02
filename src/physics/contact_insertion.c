#include "contact_insertion.h"

void contactInsert(struct ContactManifold* contactState, struct EpaResult* epaResult) {
    int shouldReplace = 1;
    int replacementIndex = 0;
    float smallestOverlap = -10000.0f;

    int insertIndex;

    for (insertIndex = 0; insertIndex < contactState->contactCount; ++insertIndex) {
        struct ContactPoint* contactPoint = &contactState->contacts[insertIndex];

        if (contactPoint->id == epaResult->id) {
            break;
        }

        if (contactPoint->penetration > smallestOverlap) {
            replacementIndex = insertIndex;
            smallestOverlap = contactPoint->penetration;
        }

        // if an existing point already exists for the faces this point covers
        // and that point contacts other faces it is a better choice than 
        // the current point
        if ((contactPoint->id & epaResult->id) == epaResult->id && contactPoint->id > epaResult->id) {
            shouldReplace = 0;
        }
    }

    contactState->normal = epaResult->normal;

    if (insertIndex == MAX_CONTACT_COUNT) {
        if (!shouldReplace) {
            return;
        }

        insertIndex = replacementIndex;
    } else {
        shouldReplace = 0;
    }

    struct ContactPoint* contactPoint = &contactState->contacts[insertIndex];

    contactPoint->id = epaResult->id;
    contactPoint->ra = epaResult->contactA;
    contactPoint->rb = epaResult->contactB;
    contactPoint->penetration = epaResult->penetration;

    if (insertIndex == contactState->contactCount) {
        contactState->contactCount = insertIndex + 1;
    }

    if (shouldReplace) {
        contactPoint->normalImpulse = 0.0f;
        contactPoint->tangentImpulse[0] = 0.0f;
        contactPoint->tangentImpulse[1] = 0.0f;
        contactPoint->bias = 0.0f;
        contactPoint->normalMass = 0.0f;
        contactPoint->tangentMass[0] = 0.0f;
        contactPoint->tangentMass[1] = 0.0f;
    }
}
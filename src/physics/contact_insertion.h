#ifndef __CONTACT_INSERTION_H__
#define __CONTACT_INSERTION_H__

#include "./contact_solver.h"
#include "./epa.h"

void contactInsert(struct ContactManifold* contactState, struct EpaResult* epa);

#endif
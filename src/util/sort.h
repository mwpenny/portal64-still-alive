#ifndef __UTIL_SORT_H__
#define __UTIL_SORT_H__

#include <stdint.h>

struct SortNode {
    uint8_t index;
    uint8_t sortOrder;
};

void mergeSort(struct SortNode* result, struct SortNode* tmp, int start, int end);

#endif

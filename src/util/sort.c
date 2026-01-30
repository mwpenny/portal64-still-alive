#include "sort.h"

void mergeSort(struct SortNode* result, struct SortNode* tmp, int start, int end) {
    if (start + 1 >= end) {
        return;
    }

    int mid = (start + end) >> 1;

    mergeSort(result, tmp, start, mid);
    mergeSort(result, tmp, mid, end);

    int currentOut = start;
    int aRead = start;
    int bRead = mid;

    while (aRead < mid || bRead < end) {
        if (bRead == end || (aRead < mid && result[aRead].sortOrder < result[bRead].sortOrder)) {
            tmp[currentOut] = result[aRead];
            ++currentOut;
            ++aRead;
        } else {
            tmp[currentOut] = result[bRead];
            ++currentOut;
            ++bRead;
        }
    }

    for (int i = start; i < end; ++i) {
        result[i] = tmp[i];
    }
}

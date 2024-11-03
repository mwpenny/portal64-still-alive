#include "string.h"

size_t strLength(char* s) {
    int size = 0;

    while (*s++ != '\0') {
        ++size;
    }

    return size;
}

char* strCopy(char* dest, const char* src) {
    while ((*dest++ = *src++) != '\0');

    return dest;
}

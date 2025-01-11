#include "string.h"

size_t strLength(char* s) {
    size_t size = 0;

    while (*s++ != '\0') {
        ++size;
    }

    return size;
}

char* strCopy(char* dest, const char* src) {
    while ((*dest++ = *src++) != '\0');

    return dest;
}

int strnCmp(const char* s1, const char* s2, size_t len) {
    while (len-- > 0) {
        unsigned char c1 = (unsigned char)*s1++;
        unsigned char c2 = (unsigned char)*s2++;

        if (c1 != c2 || c1 == '\0') {
            return c1 - c2;
        }
    }

    return 0;
}

char* strTok(char* str, const char* delim) {
    static char *curr_str;

    if (str == NULL) {
        if (curr_str == NULL) {
            return NULL;
        }

        str = curr_str;
    }

    // Find start of token (strip leading delimiters)
    char* token = NULL;
    for (char c = *str++; c != '\0' && !token; c = *str++) {
        const char* d;
        for (d = delim; *d != '\0'; ++d) {
            if (c == *d) {
                break;
            }
        }

        if (*d == '\0') {
            token = str - 1;
        }
    }

    if (token == NULL) {
        // No tokens
        curr_str = NULL;
        return token;
    }

    // Find end of token
    for (char c = *str; c != '\0'; c = *++str) {
        for (const char* d = delim; *d != '\0'; ++d) {
            if (c == *d) {
                *str = '\0';
                curr_str = str + 1;
                return token;
            }
        }
    }

    curr_str = NULL;
    return token;
}

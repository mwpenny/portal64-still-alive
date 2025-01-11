#ifndef __UTIL_STRING_H__
#define __UTIL_STRING_H__

#include <stddef.h>

size_t strLength(char* s);
char* strCopy(char* dest, const char* src);
int strnCmp(const char* s1, const char* s2, size_t len);
char* strTok(char* str, const char* delim);

#endif

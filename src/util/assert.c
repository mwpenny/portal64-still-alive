
#include "assert.h"

#if !(NDEBUG)

asm(
".global __assert\n"
".balign 4\n"
"__assert:\n"
    "teq $a0, $0\n"
    "jr $ra\n"
    "nop\n"
);

#endif
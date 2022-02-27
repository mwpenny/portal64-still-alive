
#ifndef _ASSERT_H
#define _ASSERT_H

#if NDEBUG
    #define __assert(assertion)
#else
    void __assert(int assertion);
#endif

#endif
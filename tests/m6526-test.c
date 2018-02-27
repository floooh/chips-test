//------------------------------------------------------------------------------
//  m6526-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/m6526.h"
#include <stdio.h>

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

int main() {
    // FIXME
    return 0;
}


#pragma once
/* simple test helper functions */
#include <stdio.h>
#include <stdbool.h>

#if !defined(NDEBUG)
/* debug mode, use asserts to quit right when test fails */
#include <assert.h>
#define T(x) { assert(x); _test_state.num_success++; };
#else
/* release mode, just keep track of success/fail */
#define T(x) {if(x){_test_state.num_success++;}else{printf("BLAAAAAAAA");_test_state.num_fail++;};}
#endif

typedef struct {
    const char* name;
    int num_success;
    int num_fail;
    int all_success;
    int all_fail;
} _test_state_t;
static _test_state_t _test_state;

void test_begin(const char* name) {
    assert(name);
    _test_state.name = name;
    _test_state.num_success = 0;
    _test_state.num_fail = 0;
    _test_state.all_success = 0;
    _test_state.all_fail = 0;
    printf("### RUNNING TEST SUITE '%s':\n\n", name);
}

static void _test_dump_previous(void) {
    if ((_test_state.num_fail + _test_state.num_success) > 0) {
        if (_test_state.num_fail == 0) {
            printf("    %d succeeded\n", _test_state.num_success);
        }
        else {
            printf("    *** FAIL! %d failed, %d succeeded\n", _test_state.num_fail, _test_state.num_success);
        }
    }
}

int test_end(void) {
    assert(_test_state.name);
    _test_dump_previous();
    if (_test_state.all_fail != 0) {
        printf("%s FAILED: %d failed, %d succeeded\n",
            _test_state.name, _test_state.all_fail, _test_state.all_success);
        return 10;
    }
    else {
        printf("%s SUCCESS: %d tests succeeded\n", _test_state.name, _test_state.all_success);
        return 0;
    }
}

void test(const char* name) {
    assert(name);
    assert(_test_state.name);
    _test_dump_previous();
    printf("  >> %s:\n", name);
    _test_state.all_success += _test_state.num_success;
    _test_state.all_fail += _test_state.num_fail;
    _test_state.num_success = 0;
    _test_state.num_fail = 0;
}

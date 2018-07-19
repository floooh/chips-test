#pragma once
/*
    Command line / URL argument helpers. On native platforms,
    args must be of the form "key=value" without spaces!
*/
#include <stdbool.h>
extern void args_init(int argc, char* argv[]);
extern bool args_has(const char* key);
extern const char* args_string(const char* key);
extern bool args_bool(const char* key);

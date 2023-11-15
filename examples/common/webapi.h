#pragma once
#include <stdint.h>
#include <stddef.h>
#include "chips/chips_common.h"

typedef bool (*webapi_quickload_t)(chips_range_t);

typedef struct {
    void (*boot)(void);
    void (*reset)(void);
    bool (*quickload)(chips_range_t);
} webapi_interface_t;

typedef struct {
    webapi_interface_t funcs;
} webapi_desc_t;

void webapi_init(const webapi_desc_t* desc);
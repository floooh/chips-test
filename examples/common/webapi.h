#pragma once
#include <stdint.h>
#include <stddef.h>
#include "chips/chips_common.h"

typedef struct {
    void (*boot)(void);
    void (*reset)(void);
    bool (*quickload)(chips_range_t data, bool start, bool stop_on_entry);
} webapi_interface_t;

typedef struct {
    webapi_interface_t funcs;
} webapi_desc_t;

void webapi_init(const webapi_desc_t* desc);
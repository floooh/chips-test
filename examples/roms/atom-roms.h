#pragma once
// #version:5#
// machine generated, do not edit!
#include <stdint.h>
extern unsigned char dump_abasic[8192];
extern unsigned char dump_afloat[4096];
extern unsigned char dump_dosrom[4096];
typedef struct { const char* name; const uint8_t* ptr; int size; } dump_item;
#define DUMP_NUM_ITEMS (3)
extern dump_item dump_items[DUMP_NUM_ITEMS];

//------------------------------------------------------------------------------
//  z80-fuse.c
//
//  Run the FUSE ZX Spectrum emulator CPU test:
//  https://github.com/tom-seddon/fuse-emulator-code/tree/master/fuse/z80/tests
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/z80.h"
#include <stdio.h>
#include <inttypes.h>

/* CPU state */
typedef struct {
    uint16_t af, bc, de, hl;
    uint16_t af_, bc_, de_, hl_;
    uint16_t ix, iy, sp, pc;
    uint8_t i, r, iff1, iff2, im;
    uint8_t halted;
    int ticks;
} fuse_state_t;

/* memory chunk */
#define FUSE_MAX_BYTES (32)
typedef struct {
    int num_bytes;
    uint16_t addr;
    uint8_t bytes[FUSE_MAX_BYTES];
} fuse_mem_t;

/* a result event */
typedef enum {
    EVENT_NONE,
    EVENT_MR,     /* memory read */
    EVENT_MW,     /* memory write */
    EVENT_PR,     /* port read */
    EVENT_PW,     /* port write */
} fuse_eventtype_t;

typedef struct {
    int tick;
    fuse_eventtype_t type;
    uint16_t addr;
    uint8_t data;
} fuse_event_t;

/* a complete test (used for input, output, expected) */
#define FUSE_MAX_EVENTS (128)
#define FUSE_MAX_MEMCHUNKS (8)
typedef struct {
    const char* desc;
    uint8_t num_events;
    uint8_t num_chunks;
    fuse_state_t state;
    fuse_event_t events[FUSE_MAX_EVENTS];
    fuse_mem_t chunks[FUSE_MAX_MEMCHUNKS];
} fuse_test_t;

#include "fuse/fuse.h"

int main() {
    return 0;
}
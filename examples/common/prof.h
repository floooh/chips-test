/*
    A simple profiling helper module.
*/
typedef enum {
    PROF_FRAME,     // frame time
    PROF_EMU,       // emulator time
    PROF_NUM_BUCKET_TYPES,
} prof_bucket_type_t;

typedef struct {
    int count;
    float avg_val;
    float min_val;
    float max_val;
} prof_stats_t;

// initialize profiling system
void prof_init(void);
// push a value into a profiler bucket
void prof_push(prof_bucket_type_t type, float val);
// get number of values in profiler bucket
int prof_count(prof_bucket_type_t type);
// get a value from profiler bucket
float prof_value(prof_bucket_type_t type, int index);
// get average value in bucket
prof_stats_t prof_stats(prof_bucket_type_t type);

/*== IMPLEMENTATION ==========================================================*/
#ifdef COMMON_IMPL
#include <assert.h>

#define PROF_BUCKET_SIZE (128)

// a simple ring buffer struct
typedef struct {
    int head;  // next slot to write to
    int tail;  // oldest valid slot
    float values[PROF_BUCKET_SIZE];
} prof_ring_t;

typedef struct {
    prof_ring_t ring;
} prof_bucket_t;

static struct {
    bool valid;
    prof_bucket_t buckets[PROF_NUM_BUCKET_TYPES];
} prof;

static int prof_ring_idx(int i) {
    return (i % PROF_BUCKET_SIZE);
}

static int prof_ring_count(const prof_ring_t* ring) {
    if (ring->head >= ring->tail) {
        return ring->head - ring->tail;
    }
    else {
        return (ring->head + PROF_BUCKET_SIZE) - ring->tail;
    }
}

static void prof_ring_put(prof_ring_t* ring, float value) {
    ring->values[ring->head] = value;
    ring->head = prof_ring_idx(ring->head + 1);
    if (ring->head == ring->tail) {
        ring->tail = prof_ring_idx(ring->tail + 1);
    }
}

static float prof_ring_get(prof_ring_t* ring, int index) {
    return ring->values[prof_ring_idx(ring->tail + index)];
}

// public API functions
void prof_init(void) {
    memset(&prof, 0, sizeof(prof));
    prof.valid = true;
}

void prof_push(prof_bucket_type_t type, float val) {
    assert(prof.valid);
    assert((type >= 0) && (type < PROF_NUM_BUCKET_TYPES));
    prof_ring_put(&prof.buckets[type].ring, val);
}

int prof_count(prof_bucket_type_t type) {
    assert(prof.valid);
    assert((type >= 0) && (type < PROF_NUM_BUCKET_TYPES));
    return prof_ring_count(&prof.buckets[type].ring);
}

float prof_value(prof_bucket_type_t type, int index) {
    assert(prof.valid);
    assert((type >= 0) && (type < PROF_NUM_BUCKET_TYPES));
    return prof_ring_get(&prof.buckets[type].ring, index);
}

prof_stats_t prof_stats(prof_bucket_type_t type) {
    assert(prof.valid);
    assert((type >= 0) && (type < PROF_NUM_BUCKET_TYPES));
    prof_stats_t stats = {0};
    prof_ring_t* ring = &prof.buckets[type].ring;
    stats.count = prof_ring_count(ring);
    if (stats.count > 0) {
        stats.min_val = 1000.0f;
        for (int i = 0; i < stats.count; i++) {
            float val = prof_ring_get(ring, i);
            stats.avg_val += val;
            if (val < stats.min_val) {
                stats.min_val = val;
            }
            if (val > stats.max_val) {
                stats.max_val = val;
            }
        }
        stats.avg_val /= (float)stats.count;
    }
    return stats;
}
#endif // COMMON_IMPL

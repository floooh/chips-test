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

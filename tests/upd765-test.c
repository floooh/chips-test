//------------------------------------------------------------------------------
//  upd765-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/upd765.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

static upd765_t upd;
static int cur_track[4];

static uint8_t status(void) {
    uint64_t pins = UPD765_CS|UPD765_RD;
    pins = upd765_iorq(&upd, pins);
    return UPD765_GET_DATA(pins);
}

static void wr(uint8_t val) {
    uint64_t pins = UPD765_CS|UPD765_WR|UPD765_A0;
    UPD765_SET_DATA(pins, val);
    upd765_iorq(&upd, pins);
}

static uint8_t rd(void) {
    uint64_t pins = UPD765_CS|UPD765_RD|UPD765_A0;
    pins = upd765_iorq(&upd, pins);
    return UPD765_GET_DATA(pins);
}

static int seek_track(int drive, int track, void* user_data) {
    (void)user_data;
    assert((drive >= 0) && (drive < 4));
    cur_track[drive] = track;
    return UPD765_RESULT_SUCCESS;
}

static int seek_sector(int drive, int side, upd765_sectorinfo_t* inout_info, void* user_data) {
    // FIXME
    (void)drive;
    (void)side;
    (void)inout_info;
    (void)user_data;
    return UPD765_RESULT_SUCCESS;
}

static int read_data(int drive, int side, void* user_data, uint8_t* out_data) {
    (void)drive;
    (void)side;
    (void)user_data;
    *out_data = 0;
    return UPD765_RESULT_SUCCESS;
}

static int trackinfo(int drive, int side, void* user_data, upd765_sectorinfo_t* out_info) {
    (void)user_data;
    out_info->physical_track = cur_track[drive];
    out_info->c = cur_track[drive];
    out_info->h = side;
    out_info->r = 0xC1;
    out_info->n = 2;
    out_info->st1 = 0;
    out_info->st2 = 0;
    return UPD765_RESULT_SUCCESS;
}

static void driveinfo(int drive, void* user_data, upd765_driveinfo_t* out_info) {
    (void)user_data;
    out_info->physical_track = cur_track[drive];
    out_info->sides = 1;
    out_info->head = 0;
    out_info->ready = true;
    out_info->write_protected = false;
    out_info->fault = false;
}

static void init() {
    upd765_init(&upd, &(upd765_desc_t){
        .seektrack_cb = seek_track,
        .seeksector_cb = seek_sector,
        .trackinfo_cb = trackinfo,
        .read_cb = read_data,
        .driveinfo_cb = driveinfo
    });
}

UTEST(upd765, init) {
    init();
        T(upd.phase == UPD765_PHASE_IDLE);
        T(UPD765_PHASE_IDLE == upd.phase);
        T(0 == upd.fifo_pos);
        T(UPD765_STATUS_RQM == status());
}

UTEST(upd765, invalid_command) {
    init();
    wr(0x01);
        T(UPD765_PHASE_RESULT == upd.phase);
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB|UPD765_STATUS_DIO) == status());
        T(0x80 == upd.fifo[0]);
    T(0x80 == rd());
        T(UPD765_PHASE_IDLE == upd.phase);
        T(UPD765_STATUS_RQM == status());
}

UTEST(upd765, specify_command) {
    init();
    /* this is what the Amstrad CPC sends after boot to configure the floppy controller */
    wr(0x03);
        T(UPD765_PHASE_COMMAND == upd.phase);
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB) == status());
        T(0x03 == upd.fifo[0]);
        T(1 == upd.fifo_pos);
    wr(0xA1);
        T(UPD765_PHASE_COMMAND == upd.phase);
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB) == status());
        T(0xA1 == upd.fifo[1]);
        T(2 == upd.fifo_pos);
    wr(0x03);
        T(UPD765_PHASE_IDLE == upd.phase);
        T(UPD765_STATUS_RQM == status());
        T(0x03 == upd.fifo[2]);
        T(3 == upd.fifo_pos);
}

#define DO_RECALIBRATE() \
    cur_track[0] = 10; \
    wr(0x07); \
        T(UPD765_PHASE_COMMAND == upd.phase); \
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB) == status()); \
        T(0x07 == upd.fifo[0]); \
        T(1 == upd.fifo_pos); \
    wr(0x00); \
        T(UPD765_PHASE_IDLE == upd.phase); \
        T(UPD765_STATUS_RQM == status()); \
        T(UPD765_ST0_SE == upd.st[0]); \
    T(cur_track[0] == 0);

#define DO_SENSE_INTERRUPT_STATUS() \
    wr(0x08); \
        T(UPD765_PHASE_RESULT == upd.phase); \
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB|UPD765_STATUS_DIO) == status()); \
        T(UPD765_ST0_SE == upd.st[0]); \
        T(2 == upd.fifo_num); \
        T(UPD765_ST0_SE == upd.fifo[0]); \
        T(0 == upd.fifo[1]); \
    T(UPD765_ST0_SE == rd()); \
    T(0 == rd()); \
        T(UPD765_PHASE_IDLE == upd.phase); \
        T(UPD765_STATUS_RQM == status());

#define DO_READ_ID() \
    wr(0x0A); \
        T(UPD765_PHASE_COMMAND == upd.phase); \
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB) == status()); \
        T(0x0A == upd.fifo[0]); \
        T(1 == upd.fifo_pos); \
        T(2 == upd.fifo_num); \
    wr(0x00); \
        T(UPD765_PHASE_RESULT == upd.phase); \
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB|UPD765_STATUS_DIO) == status()); \
        T(7 == upd.fifo_num); \

UTEST(upd765, recalibrate) {
    init();
    DO_RECALIBRATE();
}

UTEST(upd765, sense_interrupt_status) {
    init();
    DO_RECALIBRATE();
    DO_SENSE_INTERRUPT_STATUS();
}

UTEST(upd765, read_id) {
    init();
    DO_RECALIBRATE();
    DO_SENSE_INTERRUPT_STATUS();
    DO_READ_ID();
}

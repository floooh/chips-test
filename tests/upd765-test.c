//------------------------------------------------------------------------------
//  upd765-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/upd765.h"
#include "test.h"

upd765_t upd;
int cur_track[4];

uint8_t status(void) {
    uint64_t pins = UPD765_CS|UPD765_RD;
    pins = upd765_iorq(&upd, pins);
    return UPD765_GET_DATA(pins);
}

void wr(uint8_t val) {
    uint64_t pins = UPD765_CS|UPD765_WR|UPD765_A0;
    UPD765_SET_DATA(pins, val);
    upd765_iorq(&upd, pins);
}

uint8_t rd(void) {
    uint64_t pins = UPD765_CS|UPD765_RD|UPD765_A0;
    pins = upd765_iorq(&upd, pins);
    return UPD765_GET_DATA(pins);
}

int seek_track(int drive, int track, void* user_data) {
    assert((drive >= 0) && (drive < 4));
    cur_track[drive] = track;
    return UPD765_RESULT_SUCCESS;
}

int seek_sector(int drive, upd765_sectorinfo_t* inout_info, void* user_data) {
    // FIXME
    return UPD765_RESULT_SUCCESS;
}

int read_data(int drive, uint8_t h, void* user_data, uint8_t* out_data) {
    *out_data = 0;
    return UPD765_RESULT_SUCCESS;
}

int trackinfo(int drive, int side, void* user_data, upd765_sectorinfo_t* out_info) {
    out_info->physical_track = cur_track[drive];
    out_info->c = cur_track[drive];
    out_info->h = side;
    out_info->r = 0xC1;
    out_info->n = 2;
    out_info->st1 = 0;
    out_info->st2 = 0;
    return UPD765_RESULT_SUCCESS;
}

void init() {
    upd765_init(&upd, &(upd765_desc_t){
        .seektrack_cb = seek_track,
        .seeksector_cb = seek_sector,
        .trackinfo_cb = trackinfo,
        .read_cb = read_data
    });
    T(UPD765_PHASE_IDLE == upd.phase);
    T(0 == upd.fifo_pos);
    T(UPD765_STATUS_RQM == status());
}

void test_init(void) {
    test("init");
    init();
        T(upd.phase == UPD765_PHASE_IDLE);
}

void test_invalid(void) {
    test("invalid command");
    init();
    wr(0x01);
        T(UPD765_PHASE_RESULT == upd.phase);
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB|UPD765_STATUS_DIO) == status());
        T(0x80 == upd.fifo[0]);
    T(0x80 == rd());
        T(UPD765_PHASE_IDLE == upd.phase);
        T(UPD765_STATUS_RQM == status());
}

void test_specify(void) {
    test("specify command");
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

void do_recalibrate(void) {
    cur_track[0] = 10;
    wr(0x07);
        T(UPD765_PHASE_COMMAND == upd.phase);
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB) == status());
        T(0x07 == upd.fifo[0]);
        T(1 == upd.fifo_pos);
    wr(0x00);
        T(UPD765_PHASE_IDLE == upd.phase);
        T(UPD765_STATUS_RQM == status());
        T(UPD765_ST0_SE == upd.st[0]);
    T(cur_track[0] == 0);
}

void do_sense_interrupt_status(void) {
    wr(0x08);
        T(UPD765_PHASE_RESULT == upd.phase);
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB|UPD765_STATUS_DIO) == status());
        T(UPD765_ST0_SE == upd.st[0]);
        T(2 == upd.fifo_num);
        T(UPD765_ST0_SE == upd.fifo[0]);
        T(0 == upd.fifo[1]);
    T(UPD765_ST0_SE == rd());
    T(0 == rd());
        T(UPD765_PHASE_IDLE == upd.phase);
        T(UPD765_STATUS_RQM == status());
}

void do_read_id(void) {
    wr(0x0A);
        T(UPD765_PHASE_COMMAND == upd.phase);
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB) == status());
        T(0x0A == upd.fifo[0]);
        T(1 == upd.fifo_pos);
        T(2 == upd.fifo_num);
    wr(0x00);
        T(UPD765_PHASE_RESULT == upd.phase);
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB|UPD765_STATUS_DIO) == status());
        T(7 == upd.fifo_num);
}

void test_recalibrate(void) {
    test("recalibrate command");
    init();
    do_recalibrate();
}

void test_sense_interrupt_status(void) {
    test("sense interrupt status command");
    init();
    do_recalibrate();
    do_sense_interrupt_status();
}

void test_read_id(void) {
    test("read id command");
    init();
    do_recalibrate();
    do_sense_interrupt_status();
    do_read_id();
}

int main() {
    test_begin("UPD765 test");
    test_init();
    test_invalid();
    test_specify();
    test_recalibrate();
    test_sense_interrupt_status();
    test_read_id();

    return test_end();
}

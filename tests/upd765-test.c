//------------------------------------------------------------------------------
//  upd765-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/upd765.h"
#include "test.h"

upd765_t upd;

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

bool info(int drive, int side, int track, void* user_data, upd765_track_info_t* out_info) {
    // FIXME
    out_info->side = side;
    out_info->track = track;
    out_info->sector_id = 0xC1;
    out_info->sector_size = 2;
    out_info->st1 = 0;
    out_info->st2 = 0;
    return true;
}

void tick(void) {
    upd765_tick(&upd);
}

void init() {
    upd765_init(&upd, &(upd765_desc_t){
        .info_cb = info
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
    upd.fdd[0].track = 10;
    wr(0x07);
        T(UPD765_PHASE_COMMAND == upd.phase);
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB) == status());
        T(0x07 == upd.fifo[0]);
        T(1 == upd.fifo_pos);
    wr(0x00);
        T(UPD765_PHASE_EXECUTE == upd.phase);
        T((UPD765_STATUS_CB|UPD765_STATUS_EXM|UPD765_STATUS_RQM) == status());
        T(0x00 == upd.fifo[1]);
        T(2 == upd.fifo_pos);
    tick();
        T(UPD765_PHASE_IDLE == upd.phase);
        T(UPD765_STATUS_RQM == status());
        T(UPD765_ST0_SE == upd.st[0]);
        T(0 == upd.fdd[0].track);
}

void do_sense_interrupt_status(void) {
    wr(0x08);
        T(UPD765_PHASE_RESULT == upd.phase);
        T((UPD765_STATUS_RQM|UPD765_STATUS_CB|UPD765_STATUS_DIO) == status());
        T(0 == upd.st[0]);
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
        T((UPD765_STATUS_CB|UPD765_STATUS_EXM|UPD765_STATUS_RQM) == status());
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

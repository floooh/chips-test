//------------------------------------------------------------------------------
//  upd765-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/upd765.h"
#include "test.h"

uint16_t _read_cb(int drive, int side, int track, int sector, int index) {
    return 0;
}

uint16_t _write_cb(int drive, int side, int track, int sector, int index, uint8_t val) {
    return 0;
}

void test_init(void) {
    test("upd765 init");
    upd765_t upd;
    upd765_init(&upd, &(upd765_desc_t){
        .read_cb = _read_cb,
        .write_cb = _write_cb
    });
    T(upd.read_cb == _read_cb);
    T(upd.write_cb == _write_cb);
    T(upd.phase == UPD765_PHASE_COMMAND);
}

int main() {
    test_begin("UPD765 test");
    test_init();
    return test_end();
}
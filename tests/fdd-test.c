//------------------------------------------------------------------------------
//  fdd-test.c
//------------------------------------------------------------------------------
#include "chips/chips_common.h"
#define CHIPS_IMPL
#include "chips/fdd.h"
#include "chips/fdd_cpc.h"
#include "disks/fdd-test.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

UTEST(fdd, load_cpc_dsk) {
    fdd_t fdd;
    fdd_init(&fdd);
    bool load_successful = fdd_cpc_insert_dsk(&fdd, (chips_range_t){ .ptr=dump_boulderdash_cpc_dsk, .size=sizeof(dump_boulderdash_cpc_dsk) });
    T(load_successful);
    T(fdd.has_disc);
    T(fdd.disc.formatted);
    T(1 == fdd.disc.num_sides);
    T(40 == fdd.disc.num_tracks);
    T(fdd.data_size == (int)sizeof(dump_boulderdash_cpc_dsk));
    for (int ti = 0; ti < fdd.disc.num_tracks; ti++) {
        const fdd_track_t* track = &fdd.disc.tracks[0][ti];
        T(track->data_offset == 0x100 + 0x1300*ti);
        T(track->data_size == 0x1300);
        T(track->num_sectors == 9);
        for (int si = 0; si < 9; si++) {
            const fdd_sector_t* sector = &fdd.disc.tracks[0][ti].sectors[si];
            T((track->data_offset + 0x100 + si*0x200) == sector->data_offset);
            T(0x200 == sector->data_size);
            T(ti == sector->info.upd765.c);
            T(0 == sector->info.upd765.h);
            T(0xC1+si == sector->info.upd765.r);
            T(2 == sector->info.upd765.n);
            T(0 == sector->info.upd765.st1);
            T(0 == sector->info.upd765.st2);
        }
    }
}

UTEST(fdd, load_cpc_extdsk) {
    fdd_t fdd;
    fdd_init(&fdd);
    bool load_success = fdd_cpc_insert_dsk(&fdd, (chips_range_t){ .ptr=dump_dtc_cpc_dsk, .size=sizeof(dump_dtc_cpc_dsk) });
    T(load_success);
    T(fdd.has_disc);
    T(fdd.disc.formatted);
    T(1 == fdd.disc.num_sides);
    T(42 == fdd.disc.num_tracks);
    T(fdd.data_size == (int)sizeof(dump_dtc_cpc_dsk));
    for (int ti = 0; ti < fdd.disc.num_tracks; ti++) {
        const fdd_track_t* track = &fdd.disc.tracks[0][ti];
        T(track->data_offset == 0x100 + 0x1300*ti);
        T(track->data_size == 0x1300);
        T(track->num_sectors == 9);
        for (int si = 0; si < 9; si++) {
            const fdd_sector_t* sector = &fdd.disc.tracks[0][ti].sectors[si];
            T((track->data_offset + 0x100 + si*0x200) == sector->data_offset);
            T(0x200 == sector->data_size);
            T(ti == sector->info.upd765.c);
            T(0 == sector->info.upd765.h);
            if (si & 1) {
                T(0xC6+si/2 == sector->info.upd765.r);
            }
            else {
                T(0xC1+si/2 == sector->info.upd765.r);
            }
            T(2 == sector->info.upd765.n);
            T(0 == sector->info.upd765.st1);
            T(0 == sector->info.upd765.st2);
        }
    }
}

/*
    zx.c
    ZX Spectrum 48/128 emulator.
    - wait states when accessing contended memory are not emulated
    - video decoding works with scanline accuracy, not cycle accuracy
    - no tape or disc emulation
*/
#include "sokol_app.h"
#include "sokol_audio.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/beeper.h"
#include "chips/ay38910.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/zx.h"
#include "common/gfx.h"
#include "common/clock.h"
#include "common/fs.h"
#include "common/args.h"
#include "roms/zx-roms.h"

zx_t zx;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    args_init(argc, argv);
    fs_init();
    if (args_has("file")) {
        fs_load_file(args_string("file"));
    }
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * ZX_DISPLAY_WIDTH,
        .height = 2 * ZX_DISPLAY_HEIGHT,
        .window_title = "ZX Spectrum",
        .ios_keyboard_resizes_canvas = true
    };
}

/* one-time application init */
void app_init() {
    gfx_init(ZX_DISPLAY_WIDTH, ZX_DISPLAY_HEIGHT, 1, 1);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    zx_type_t type = ZX_TYPE_128;
    if (args_has("type")) {
        if (args_string_compare("type", "zx48k")) {
            type = ZX_TYPE_48K;
        }
    }
    zx_joystick_t joy_type = ZX_JOYSTICK_NONE;
    if (args_has("joystick")) {
        if (args_string_compare("joystick", "kempston")) {
            joy_type = ZX_JOYSTICK_KEMPSTON;
        }
        else if (args_string_compare("joystick", "sinclair1")) {
            joy_type = ZX_JOYSTICK_SINCLAIR_1;
        }
        else if (args_string_compare("joystick", "sinclair2")) {
            joy_type = ZX_JOYSTICK_SINCLAIR_2;
        }
    }
    zx_init(&zx, &(zx_desc_t) {
        .type = type,
        .joystick_type = joy_type,
        .pixel_buffer = rgba8_buffer,
        .pixel_buffer_size = sizeof(rgba8_buffer),
        .audio_cb = saudio_push,
        .audio_sample_rate = saudio_sample_rate(),
        .rom_zx48k = dump_amstrad_zx48k,
        .rom_zx48k_size = sizeof(dump_amstrad_zx48k),
        .rom_zx128_0 = dump_amstrad_zx128k_0,
        .rom_zx128_0_size = sizeof(dump_amstrad_zx128k_0),
        .rom_zx128_1 = dump_amstrad_zx128k_1,
        .rom_zx128_1_size = sizeof(dump_amstrad_zx128k_1)
    });
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    zx_exec(&zx, clock_frame_time());
    gfx_draw();
    if (fs_ptr() && clock_frame_count() > 20) {
        zx_quickload(&zx, fs_ptr(), fs_size());
        fs_free();
    }
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    const bool joy_enabled = zx.joystick_type != ZX_JOYSTICK_NONE;
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if (c < KBD_MAX_KEYS) {
                kbd_key_down(&zx.kbd, c);
                kbd_key_up(&zx.kbd, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_SPACE:        c = joy_enabled ? 0 : 0x20; break;
                case SAPP_KEYCODE_LEFT:         c = joy_enabled ? 0 : 0x08; break;
                case SAPP_KEYCODE_RIGHT:        c = joy_enabled ? 0 : 0x09; break;
                case SAPP_KEYCODE_DOWN:         c = joy_enabled ? 0 : 0x0A; break;
                case SAPP_KEYCODE_UP:           c = joy_enabled ? 0 : 0x0B; break;
                case SAPP_KEYCODE_ENTER:        c = 0x0D; break;
                case SAPP_KEYCODE_BACKSPACE:    c = 0x0C; break;
                case SAPP_KEYCODE_ESCAPE:       c = 0x07; break;
                case SAPP_KEYCODE_LEFT_CONTROL: c = 0x0F; break; /* SymShift */
                default:                        c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    kbd_key_down(&zx.kbd, c);
                }
                else {
                    kbd_key_up(&zx.kbd, c);
                }
            }
            if (joy_enabled) {
                if (zx.joystick_type == ZX_JOYSTICK_KEMPSTON) {
                    if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    zx.kempston_mask |= 1<<4; break;
                            case SAPP_KEYCODE_LEFT:     zx.kempston_mask |= 1<<1; break;
                            case SAPP_KEYCODE_RIGHT:    zx.kempston_mask |= 1<<0; break;
                            case SAPP_KEYCODE_DOWN:     zx.kempston_mask |= 1<<2; break;
                            case SAPP_KEYCODE_UP:       zx.kempston_mask |= 1<<3; break;
                            default: break;
                        }
                    }
                    else {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    zx.kempston_mask &= ~(1<<4); break;
                            case SAPP_KEYCODE_LEFT:     zx.kempston_mask &= ~(1<<1); break;
                            case SAPP_KEYCODE_RIGHT:    zx.kempston_mask &= ~(1<<0); break;
                            case SAPP_KEYCODE_DOWN:     zx.kempston_mask &= ~(1<<2); break;
                            case SAPP_KEYCODE_UP:       zx.kempston_mask &= ~(1<<3); break;
                            default: break;
                        }
                    }
                }
                else if (zx.joystick_type == ZX_JOYSTICK_SINCLAIR_1) {
                    if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    kbd_key_down(&zx.kbd, '5'); break;
                            case SAPP_KEYCODE_LEFT:     kbd_key_down(&zx.kbd, '1'); break;
                            case SAPP_KEYCODE_RIGHT:    kbd_key_down(&zx.kbd, '2'); break;
                            case SAPP_KEYCODE_DOWN:     kbd_key_down(&zx.kbd, '3'); break;
                            case SAPP_KEYCODE_UP:       kbd_key_down(&zx.kbd, '4'); break;
                            default: break;
                        }
                    }
                    else {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    kbd_key_up(&zx.kbd, '5'); break;
                            case SAPP_KEYCODE_LEFT:     kbd_key_up(&zx.kbd, '1'); break;
                            case SAPP_KEYCODE_RIGHT:    kbd_key_up(&zx.kbd, '2'); break;
                            case SAPP_KEYCODE_DOWN:     kbd_key_up(&zx.kbd, '3'); break;
                            case SAPP_KEYCODE_UP:       kbd_key_up(&zx.kbd, '4'); break;
                            default: break;
                        }
                    }
                }
                else if (zx.joystick_type == ZX_JOYSTICK_SINCLAIR_2) {
                    if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    kbd_key_down(&zx.kbd, '0'); break;
                            case SAPP_KEYCODE_LEFT:     kbd_key_down(&zx.kbd, '6'); break;
                            case SAPP_KEYCODE_RIGHT:    kbd_key_down(&zx.kbd, '7'); break;
                            case SAPP_KEYCODE_DOWN:     kbd_key_down(&zx.kbd, '8'); break;
                            case SAPP_KEYCODE_UP:       kbd_key_down(&zx.kbd, '9'); break;
                            default: break;
                        }
                    }
                    else {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    kbd_key_up(&zx.kbd, '0'); break;
                            case SAPP_KEYCODE_LEFT:     kbd_key_up(&zx.kbd, '6'); break;
                            case SAPP_KEYCODE_RIGHT:    kbd_key_up(&zx.kbd, '7'); break;
                            case SAPP_KEYCODE_DOWN:     kbd_key_up(&zx.kbd, '8'); break;
                            case SAPP_KEYCODE_UP:       kbd_key_up(&zx.kbd, '9'); break;
                            default: break;
                        }
                    }
                }
            }
            break;
        case SAPP_EVENTTYPE_TOUCHES_BEGAN:
            sapp_show_keyboard(true);
            break;
        default:
            break;
    }
}

/* application cleanup callback */
void app_cleanup() {
    zx_discard(&zx);
    saudio_shutdown();
    gfx_shutdown();
}

/* ZX Z80 file format header (http://www.worldofspectrum.org/faq/reference/z80format.htm ) */
typedef struct {
    uint8_t A, F;
    uint8_t C, B;
    uint8_t L, H;
    uint8_t PC_l, PC_h;
    uint8_t SP_l, SP_h;
    uint8_t I, R;
    uint8_t flags0;
    uint8_t E, D;
    uint8_t C_, B_;
    uint8_t E_, D_;
    uint8_t L_, H_;
    uint8_t A_, F_;
    uint8_t IY_l, IY_h;
    uint8_t IX_l, IX_h;
    uint8_t EI;
    uint8_t IFF2;
    uint8_t flags1;
} zx_z80_header;

typedef struct {
    uint8_t len_l;
    uint8_t len_h;
    uint8_t PC_l, PC_h;
    uint8_t hw_mode;
    uint8_t out_7ffd;
    uint8_t rom1;
    uint8_t flags;
    uint8_t out_fffd;
    uint8_t audio[16];
    uint8_t tlow_l;
    uint8_t tlow_h;
    uint8_t spectator_flags;
    uint8_t mgt_rom_paged;
    uint8_t multiface_rom_paged;
    uint8_t rom_0000_1fff;
    uint8_t rom_2000_3fff;
    uint8_t joy_mapping[10];
    uint8_t kbd_mapping[10];
    uint8_t mgt_type;
    uint8_t disciple_button_state;
    uint8_t disciple_flags;
    uint8_t out_1ffd;
} zx_z80_ext_header;

typedef struct {
    uint8_t len_l;
    uint8_t len_h;
    uint8_t page_nr;
} zx_z80_page_header;

static bool overflow(const uint8_t* ptr, intptr_t num_bytes, const uint8_t* end_ptr) {
    return (ptr + num_bytes) > end_ptr;
}

bool zx_quickload(zx_t* sys, const uint8_t* ptr, int num_bytes) {
    const uint8_t* end_ptr = ptr + num_bytes;
    if (overflow(ptr, sizeof(zx_z80_header), end_ptr)) {
        return false;
    }
    const zx_z80_header* hdr = (const zx_z80_header*) ptr;
    ptr += sizeof(zx_z80_header);
    const zx_z80_ext_header* ext_hdr = 0;
    uint16_t pc = (hdr->PC_h<<8 | hdr->PC_l) & 0xFFFF;
    const bool is_version1 = 0 != pc;
    if (!is_version1) {
        if (overflow(ptr, sizeof(zx_z80_ext_header), end_ptr)) {
            return false;
        }
        ext_hdr = (zx_z80_ext_header*) ptr;
        int ext_hdr_len = (ext_hdr->len_h<<8)|ext_hdr->len_l;
        ptr += 2 + ext_hdr_len;
        if (ext_hdr->hw_mode < 3) {
            if (zx.type != ZX_TYPE_48K) {
                return false;
            }
        }
        else {
            if (sys->type != ZX_TYPE_128) {
                return false;
            }
        }
    }
    else {
        if (sys->type != ZX_TYPE_48K) {
            return false;
        }
    }
    const bool v1_compr = 0 != (hdr->flags0 & (1<<5));
    while (ptr < end_ptr) {
        int page_index = 0;
        int src_len = 0, dst_len = 0;
        if (is_version1) {
            src_len = num_bytes - sizeof(zx_z80_header);
            dst_len = 48 * 1024;
        }
        else {
            zx_z80_page_header* phdr = (zx_z80_page_header*) ptr;
            if (overflow(ptr, sizeof(zx_z80_page_header), end_ptr)) {
                return false;
            }
            ptr += sizeof(zx_z80_page_header);
            src_len = (phdr->len_h<<8 | phdr->len_l) & 0xFFFF;
            dst_len = 0x4000;
            page_index = phdr->page_nr - 3;
            if ((sys->type == ZX_TYPE_48K) && (page_index == 5)) {
                page_index = 0;
            }
            if ((page_index < 0) || (page_index > 7)) {
                page_index = -1;
            }
        }
        uint8_t* dst_ptr;
        if (-1 == page_index) {
            dst_ptr = sys->junk;
        }
        else {
            dst_ptr = sys->ram[page_index];
        }
        if (0xFFFF == src_len) {
            // FIXME: uncompressed not supported yet
            return false;
        }
        else {
            /* compressed */
            int src_pos = 0;
            bool v1_done = false;
            uint8_t val[4];
            while ((src_pos < src_len) && !v1_done) {
                val[0] = ptr[src_pos];
                val[1] = ptr[src_pos+1];
                val[2] = ptr[src_pos+2];
                val[3] = ptr[src_pos+3];
                /* check for version 1 end marker */
                if (v1_compr && (0==val[0]) && (0xED==val[1]) && (0xED==val[2]) && (0==val[3])) {
                    v1_done = true;
                    src_pos += 4;
                }
                else if (0xED == val[0]) {
                    if (0xED == val[1]) {
                        uint8_t count = val[2];
                        assert(0 != count);
                        uint8_t data = val[3];
                        src_pos += 4;
                        for (int i = 0; i < count; i++) {
                            *dst_ptr++ = data;
                        }
                    }
                    else {
                        /* single ED */
                        *dst_ptr++ = val[0];
                        src_pos++;
                    }
                }
                else {
                    /* any value */
                    *dst_ptr++ = val[0];
                    src_pos++;
                }
            }
            assert(src_pos == src_len);
        }
        if (0xFFFF == src_len) {
            ptr += 0x4000;
        }
        else {
            ptr += src_len;
        }
    }

    /* start loaded image */
    z80_reset(&sys->cpu);
    z80_set_a(&sys->cpu, hdr->A); z80_set_f(&sys->cpu, hdr->F);
    z80_set_b(&sys->cpu, hdr->B); z80_set_c(&sys->cpu, hdr->C);
    z80_set_d(&sys->cpu, hdr->D); z80_set_e(&sys->cpu, hdr->E);
    z80_set_h(&sys->cpu, hdr->H); z80_set_l(&sys->cpu, hdr->L);
    z80_set_ix(&sys->cpu, hdr->IX_h<<8|hdr->IX_l);
    z80_set_iy(&sys->cpu, hdr->IY_h<<8|hdr->IY_l);
    z80_set_af_(&sys->cpu, hdr->A_<<8|hdr->F_);
    z80_set_bc_(&sys->cpu, hdr->B_<<8|hdr->C_);
    z80_set_de_(&sys->cpu, hdr->D_<<8|hdr->E_);
    z80_set_hl_(&sys->cpu, hdr->H_<<8|hdr->L_);
    z80_set_sp(&sys->cpu, hdr->SP_h<<8|hdr->SP_l);
    z80_set_i(&sys->cpu, hdr->I);
    z80_set_r(&sys->cpu, (hdr->R & 0x7F) | ((hdr->flags0 & 1)<<7));
    z80_set_iff2(&sys->cpu, hdr->IFF2 != 0);
    z80_set_ei_pending(&sys->cpu, hdr->EI != 0);
    if (hdr->flags1 != 0xFF) {
        z80_set_im(&sys->cpu, hdr->flags1 & 3);
    }
    else {
        z80_set_im(&sys->cpu, 1);
    }
    if (ext_hdr) {
        z80_set_pc(&sys->cpu, ext_hdr->PC_h<<8|ext_hdr->PC_l);
        if (sys->type == ZX_TYPE_128) {
            for (int i = 0; i < 16; i++) {
                /* latch AY-3-8912 register address */
                ay38910_iorq(&sys->ay, AY38910_BDIR|AY38910_BC1|(i<<16));
                /* write AY-3-8912 register value */
                ay38910_iorq(&sys->ay, AY38910_BDIR|(ext_hdr->audio[i]<<16));
            }
        }
        /* simulate an out of port 0xFFFD and 0x7FFD */
        uint64_t pins = Z80_IORQ|Z80_WR;
        Z80_SET_ADDR(pins, 0xFFFD);
        Z80_SET_DATA(pins, ext_hdr->out_fffd);
        _zx_tick(4, pins, sys);
        Z80_SET_ADDR(pins, 0x7FFD);
        Z80_SET_DATA(pins, ext_hdr->out_7ffd);
        _zx_tick(4, pins, sys);
    }
    else {
        z80_set_pc(&sys->cpu, hdr->PC_h<<8|hdr->PC_l);
    }
    sys->border_color = _zx_palette[(hdr->flags0>>1) & 7] & 0xFFD7D7D7;
    return true;
}

/*
    zx.c
    ZX Spectrum 48/128 emulator.
    - wait states when accessing contended memory are not emulated
    - video decoding works with scanline accuracy, not cycle accuracy
    - no tape or disc emulation
*/
#include "sokol_app.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/beeper.h"
#include "chips/ay38910.h"
#include "chips/mem.h"
#include "chips/kbd.h"
#include "roms/zx-roms.h"
#include "common/gfx.h"
#include "common/sound.h"
#include "common/clock.h"
#include "common/fs.h"
#include "common/args.h"

#include <stdio.h>

/* ZX Spectrum state and callbacks */
#define ZX48K_FREQ (3500000)
#define ZX128K_FREQ (3546894)
#define ZX_DISP_WIDTH (320)
#define ZX_DISP_HEIGHT (256)

typedef enum {
    ZX_TYPE_128K,
    ZX_TYPE_48K
} zx_type_t;

typedef enum {
    ZX_JOYSTICK_NONE,
    ZX_JOYSTICK_KEMPSTON,
    ZX_JOYSTICK_SINCLAIR_1,
    ZX_JOYSTICK_SINCLAIR_2,
} zx_joystick_type_t;

typedef struct {
    zx_type_t type;
    z80_t cpu;
    beeper_t beeper;
    ay38910_t ay;
    kbd_t kbd;
    mem_t mem;
    bool memory_paging_disabled;

    zx_joystick_type_t joystick_type;
    uint8_t kempston_mask;
    uint32_t tick_count;
    uint8_t last_fe_out;            // last out value to 0xFE port */
    uint8_t blink_counter;          // incremented on each vblank
    int frame_scan_lines;
    int top_border_scanlines;
    int scanline_period;
    int scanline_counter;
    int scanline_y;
    uint32_t display_ram_bank;
    uint32_t border_color;
    uint8_t ram[8][0x4000];
    uint8_t junk[0x4000];
} zx_t;
zx_t _zx;

uint32_t zx_palette[8] = {
    0xFF000000,     // black
    0xFFFF0000,     // blue
    0xFF0000FF,     // red
    0xFFFF00FF,     // magenta
    0xFF00FF00,     // green
    0xFFFFFF00,     // cyan
    0xFF00FFFF,     // yellow
    0xFFFFFFFF,     // white
};

void zx_init(zx_t* zx, zx_type_t type);
uint64_t zx_cpu_tick(int num_ticks, uint64_t pins, void* user_data);
bool zx_decode_scanline(zx_t* zx);
bool zx_load_z80(zx_t* zx, const uint8_t* ptr, uint32_t num_bytes);

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
        .width = 2 * ZX_DISP_WIDTH,
        .height = 2 * ZX_DISP_HEIGHT,
        .window_title = "ZX Spectrum",
        .ios_keyboard_resizes_canvas = true
    };
}

/* one-time application init */
void app_init() {
    zx_t* zx = &_zx;
    zx_type_t type = ZX_TYPE_128K;
    if (args_has("type")) {
        if (args_string_compare("type", "zx48k")) {
            type = ZX_TYPE_48K;
        }
    }
    gfx_init(ZX_DISP_WIDTH, ZX_DISP_HEIGHT, 1, 1);
    sound_init();
    clock_init((type == ZX_TYPE_128K) ? ZX128K_FREQ : ZX48K_FREQ);
    zx_init(zx, type);
    if (args_has("joystick")) {
        if (args_string_compare("joystick", "kempston")) {
            zx->joystick_type = ZX_JOYSTICK_KEMPSTON;
        }
        else if (args_string_compare("joystick", "sinclair1")) {
            zx->joystick_type = ZX_JOYSTICK_SINCLAIR_1;
        }
        else if (args_string_compare("joystick", "sinclair2")) {
            zx->joystick_type = ZX_JOYSTICK_SINCLAIR_2;
        }
    }
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    zx_t* zx = &_zx;
    clock_ticks_executed(z80_exec(&zx->cpu, clock_ticks_to_run()));
    kbd_update(&zx->kbd);
    gfx_draw();
    /* load snapshot file? */
    if (fs_ptr() && clock_frame_count() > 120) {
        zx_load_z80(zx, fs_ptr(), fs_size());
        fs_free();
    }
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    zx_t* zx = &_zx;
    const bool joy_enabled = zx->joystick_type != ZX_JOYSTICK_NONE;
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if (c < KBD_MAX_KEYS) {
                kbd_key_down(&zx->kbd, c);
                kbd_key_up(&zx->kbd, c);
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
                    kbd_key_down(&zx->kbd, c);
                }
                else {
                    kbd_key_up(&zx->kbd, c);
                }
            }
            if (joy_enabled) {
                if (zx->joystick_type == ZX_JOYSTICK_KEMPSTON) {
                    if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    zx->kempston_mask |= 1<<4; break;
                            case SAPP_KEYCODE_LEFT:     zx->kempston_mask |= 1<<1; break;
                            case SAPP_KEYCODE_RIGHT:    zx->kempston_mask |= 1<<0; break;
                            case SAPP_KEYCODE_DOWN:     zx->kempston_mask |= 1<<2; break;
                            case SAPP_KEYCODE_UP:       zx->kempston_mask |= 1<<3; break;
                            default: break;
                        }
                    }
                    else {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    zx->kempston_mask &= ~(1<<4); break;
                            case SAPP_KEYCODE_LEFT:     zx->kempston_mask &= ~(1<<1); break;
                            case SAPP_KEYCODE_RIGHT:    zx->kempston_mask &= ~(1<<0); break;
                            case SAPP_KEYCODE_DOWN:     zx->kempston_mask &= ~(1<<2); break;
                            case SAPP_KEYCODE_UP:       zx->kempston_mask &= ~(1<<3); break;
                            default: break;
                        }
                    }
                }
                else if (zx->joystick_type == ZX_JOYSTICK_SINCLAIR_1) {
                    if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    kbd_key_down(&zx->kbd, '5'); break;
                            case SAPP_KEYCODE_LEFT:     kbd_key_down(&zx->kbd, '1'); break;
                            case SAPP_KEYCODE_RIGHT:    kbd_key_down(&zx->kbd, '2'); break;
                            case SAPP_KEYCODE_DOWN:     kbd_key_down(&zx->kbd, '3'); break;
                            case SAPP_KEYCODE_UP:       kbd_key_down(&zx->kbd, '4'); break;
                            default: break;
                        }
                    }
                    else {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    kbd_key_up(&zx->kbd, '5'); break;
                            case SAPP_KEYCODE_LEFT:     kbd_key_up(&zx->kbd, '1'); break;
                            case SAPP_KEYCODE_RIGHT:    kbd_key_up(&zx->kbd, '2'); break;
                            case SAPP_KEYCODE_DOWN:     kbd_key_up(&zx->kbd, '3'); break;
                            case SAPP_KEYCODE_UP:       kbd_key_up(&zx->kbd, '4'); break;
                            default: break;
                        }
                    }
                }
                else if (zx->joystick_type == ZX_JOYSTICK_SINCLAIR_2) {
                    if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    kbd_key_down(&zx->kbd, '0'); break;
                            case SAPP_KEYCODE_LEFT:     kbd_key_down(&zx->kbd, '6'); break;
                            case SAPP_KEYCODE_RIGHT:    kbd_key_down(&zx->kbd, '7'); break;
                            case SAPP_KEYCODE_DOWN:     kbd_key_down(&zx->kbd, '8'); break;
                            case SAPP_KEYCODE_UP:       kbd_key_down(&zx->kbd, '9'); break;
                            default: break;
                        }
                    }
                    else {
                        switch (event->key_code) {
                            case SAPP_KEYCODE_SPACE:    kbd_key_up(&zx->kbd, '0'); break;
                            case SAPP_KEYCODE_LEFT:     kbd_key_up(&zx->kbd, '6'); break;
                            case SAPP_KEYCODE_RIGHT:    kbd_key_up(&zx->kbd, '7'); break;
                            case SAPP_KEYCODE_DOWN:     kbd_key_up(&zx->kbd, '8'); break;
                            case SAPP_KEYCODE_UP:       kbd_key_up(&zx->kbd, '9'); break;
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
    sound_shutdown();
    gfx_shutdown();
}

/* ZX Spectrum 128 emulator init */
void zx_init(zx_t* zx, zx_type_t type) {
    zx->type = type;
    zx->border_color = 0xFF000000;
    if (ZX_TYPE_128K == zx->type) {
        zx->display_ram_bank = 5;
        zx->frame_scan_lines = 311;
        zx->top_border_scanlines = 63;
        zx->scanline_period = 228;
    }
    else {
        zx->display_ram_bank = 0;
        zx->frame_scan_lines = 312;
        zx->top_border_scanlines = 64;
        zx->scanline_period = 224;
    }
    zx->scanline_counter = zx->scanline_period;

    const int freq = (zx->type == ZX_TYPE_128K) ? ZX128K_FREQ : ZX48K_FREQ;
    z80_init(&zx->cpu, &(z80_desc_t) {
        .tick_cb = zx_cpu_tick,
        .user_data = zx
    });
    beeper_init(&zx->beeper, freq, sound_sample_rate(), 0.25f);
    if (ZX_TYPE_128K == zx->type) {
        ay38910_init(&zx->ay, &(ay38910_desc_t){
            .type = AY38910_TYPE_8912,
            .tick_hz = freq/2,
            .sound_hz = sound_sample_rate(),
            .magnitude = 0.5
        });
    }
    z80_set_pc(&zx->cpu, 0x0000);

    /* initial memory map */
    if (zx->type == ZX_TYPE_128K) {
        mem_map_ram(&zx->mem, 0, 0x4000, 0x4000, zx->ram[5]);
        mem_map_ram(&zx->mem, 0, 0x8000, 0x4000, zx->ram[2]);
        mem_map_ram(&zx->mem, 0, 0xC000, 0x4000, zx->ram[0]);
        mem_map_rom(&zx->mem, 0, 0x0000, 0x4000, dump_amstrad_zx128k_0);
    }
    else {
        mem_map_ram(&zx->mem, 0, 0x4000, 0x4000, zx->ram[0]);
        mem_map_ram(&zx->mem, 0, 0x8000, 0x4000, zx->ram[1]);
        mem_map_ram(&zx->mem, 0, 0xC000, 0x4000, zx->ram[2]);
        mem_map_rom(&zx->mem, 0, 0x0000, 0x4000, dump_amstrad_zx48k);
    }

    /* setup keyboard matrix */
    kbd_init(&zx->kbd, 1);
    /* caps-shift is column 0, line 0 */
    kbd_register_modifier(&zx->kbd, 0, 0, 0);
    /* sym-shift is column 7, line 1 */
    kbd_register_modifier(&zx->kbd, 1, 7, 1);
    /* alpha-numeric keys */
    const char* keymap =
        // no shift
        " zxcv"         // A8       shift,z,x,c,v
        "asdfg"         // A9       a,s,d,f,g
        "qwert"         // A10      q,w,e,r,t
        "12345"         // A11      1,2,3,4,5
        "09876"         // A12      0,9,8,7,6
        "poiuy"         // A13      p,o,i,u,y
        " lkjh"         // A14      enter,l,k,j,h
        "  mnb"         // A15      space,symshift,m,n,b

        // shift
        " ZXCV"         // A8
        "ASDFG"         // A9
        "QWERT"         // A10
        "     "         // A11
        "     "         // A12
        "POIUY"         // A13
        " LKJH"         // A14
        "  MNB"         // A15

        // symshift
        " : ?/"         // A8
        "     "         // A9
        "   <>"         // A10
        "!@#$%"         // A11
        "_)('&"         // A12
        "\";   "        // A13
        " =+-^"         // A14
        "  .,*";        // A15
    for (int layer = 0; layer < 3; layer++) {
        for (int col = 0; col < 8; col++) {
            for (int line = 0; line < 5; line++) {
                const uint8_t c = keymap[layer*40 + col*5 + line];
                if (c != 0x20) {
                    kbd_register_key(&zx->kbd, c, col, line, (layer>0) ? (1<<(layer-1)) : 0);
                }
            }
        }
    }

    /* special keys */
    kbd_register_key(&zx->kbd, ' ', 7, 0, 0);  // Space
    kbd_register_key(&zx->kbd, 0x0F, 7, 1, 0); // SymShift
    kbd_register_key(&zx->kbd, 0x08, 3, 4, 1); // Cursor Left (Shift+5)
    kbd_register_key(&zx->kbd, 0x0A, 4, 4, 1); // Cursor Down (Shift+6)
    kbd_register_key(&zx->kbd, 0x0B, 4, 3, 1); // Cursor Up (Shift+7)
    kbd_register_key(&zx->kbd, 0x09, 4, 2, 1); // Cursor Right (Shift+8)
    kbd_register_key(&zx->kbd, 0x07, 3, 0, 1); // Edit (Shift+1)
    kbd_register_key(&zx->kbd, 0x0C, 4, 0, 1); // Delete (Shift+0)
    kbd_register_key(&zx->kbd, 0x0D, 6, 0, 0); // Enter
}

/* the CPU tick callback */
uint64_t zx_cpu_tick(int num_ticks, uint64_t pins, void* user_data) {
    zx_t* zx = (zx_t*) user_data;
    /* video decoding and vblank interrupt */
    zx->scanline_counter -= num_ticks;
    if (zx->scanline_counter <= 0) {
        zx->scanline_counter += zx->scanline_period;
        // decode next video scanline
        if (zx_decode_scanline(zx)) {
            // request vblank interrupt
            pins |= Z80_INT;
        }
    }

    /* tick audio systems */
    for (int i = 0; i < num_ticks; i++) {
        zx->tick_count++;
        bool sample_ready = beeper_tick(&zx->beeper);
        /* the AY-3-8912 chip runs at half CPU frequency */
        if (zx->type == ZX_TYPE_128K) {
            if (zx->tick_count & 1) {
                ay38910_tick(&zx->ay);
            }
        }
        if (sample_ready) {
            float sample = zx->beeper.sample;
            if (zx->type == ZX_TYPE_128K) {
                sample += zx->ay.sample;
            }
            sound_push(sample);
        }
    }

    /* memory and IO requests */
    if (pins & Z80_MREQ) {
        /* a memory request machine cycle
            FIXME: 'contended memory' accesses should inject wait states
        */
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem_rd(&zx->mem, addr));
        }
        else if (pins & Z80_WR) {
            mem_wr(&zx->mem, addr, Z80_GET_DATA(pins));
        }
    }
    else if (pins & Z80_IORQ) {
        /* an IO request machine cycle
            see http://problemkaputt.de/zxdocs.htm#zxspectrum for address decoding
        */
        if (pins & Z80_RD) {
            /* an IO read
                FIXME: reading from port xxFF should return 'current VRAM data'
            */
            if ((pins & Z80_A0) == 0) {
                /* Spectrum ULA (...............0)
                    Bits 5 and 7 as read by INning from Port 0xfe are always one
                */
                uint8_t data = (1<<7)|(1<<5);
                /* MIC/EAR flags -> bit 6 */
                if (zx->last_fe_out & (1<<3|1<<4)) {
                    data |= (1<<6);
                }
                /* keyboard matrix bits are encoded in the upper 8 bit of the port address */
                uint16_t column_mask = (~(Z80_GET_ADDR(pins)>>8)) & 0x00FF;
                const uint16_t kbd_lines = kbd_test_lines(&zx->kbd, column_mask);
                data |= (~kbd_lines) & 0x1F;
                Z80_SET_DATA(pins, data);
            }
            else if ((pins & (Z80_A7|Z80_A6|Z80_A5)) == 0) {
                /* Kempston Joystick (........000.....) */
                Z80_SET_DATA(pins, zx->kempston_mask);
            }
            else if (zx->type == ZX_TYPE_128K){
                /* read from AY-3-8912 (11............0.) */
                if ((pins & (Z80_A15|Z80_A14|Z80_A1)) == (Z80_A15|Z80_A14)) {
                    pins = ay38910_iorq(&zx->ay, AY38910_BC1|pins) & Z80_PIN_MASK;
                }
            }
        }
        else if (pins & Z80_WR) {
            // an IO write
            const uint8_t data = Z80_GET_DATA(pins);
            if ((pins & Z80_A0) == 0) {
                /* Spectrum ULA (...............0)
                    "every even IO port addresses the ULA but to avoid
                    problems with other I/O devices, only FE should be used"
                    FIXME:
                        bit 3: MIC output (CAS SAVE, 0=On, 1=Off)
                */
                zx->border_color = zx_palette[data & 7] & 0xFFD7D7D7;
                zx->last_fe_out = data;
                beeper_set(&zx->beeper, 0 != (data & (1<<4)));
            }
            else if (zx->type == ZX_TYPE_128K) {
                /* Spectrum 128 memory control (0.............0.)
                    http://8bit.yarek.pl/computer/zx.128/
                */
                if ((pins & (Z80_A15|Z80_A1)) == 0) {
                    if (!zx->memory_paging_disabled) {
                        /* bit 3 defines the video scanout memory bank (5 or 7) */
                        zx->display_ram_bank = (data & (1<<3)) ? 7 : 5;
                        /* only last memory bank is mappable */
                        mem_map_ram(&zx->mem, 0, 0xC000, 0x4000, zx->ram[data & 0x7]);

                        /* ROM0 or ROM1 */
                        if (data & (1<<4)) {
                            /* bit 4 set: ROM1 */
                            mem_map_rom(&zx->mem, 0, 0x0000, 0x4000, dump_amstrad_zx128k_1);
                        }
                        else {
                            /* bit 4 clear: ROM0 */
                            mem_map_rom(&zx->mem, 0, 0x0000, 0x4000, dump_amstrad_zx128k_0);
                        }
                    }
                    if (data & (1<<5)) {
                        /* bit 5 prevents further changes to memory pages
                            until computer is reset, this is used when switching
                            to the 48k ROM
                        */
                        zx->memory_paging_disabled = true;
                    }
                }
                else if ((pins & (Z80_A15|Z80_A14|Z80_A1)) == (Z80_A15|Z80_A14)) {
                    /* select AY-3-8912 register (11............0.) */
                    ay38910_iorq(&zx->ay, AY38910_BDIR|AY38910_BC1|pins);
                }
                else if ((pins & (Z80_A15|Z80_A14|Z80_A1)) == Z80_A15) {
                    /* write to AY-3-8912 (10............0.) */
                    ay38910_iorq(&zx->ay, AY38910_BDIR|pins);
                }
            }
        }
    }
    return pins;
}

bool zx_decode_scanline(zx_t* zx) {
    /* this is called by the timer callback for every PAL line, controlling
        the vidmem decoding and vblank interrupt

        detailed information about frame timings is here:
        for 48K:    http://rk.nvg.ntnu.no/sinclair/faq/tech_48.html#48K
        for 128K:   http://rk.nvg.ntnu.no/sinclair/faq/tech_128.html

        one PAL line takes 224 T-states on 48K, and 228 T-states on 128K
        one PAL frame is 312 lines on 48K, and 311 lines on 128K

        decode the next videomem line into the emulator framebuffer,
        the border area of a real Spectrum is bigger than the emulator
        (the emu has 32 pixels border on each side, the hardware has:

        63 or 64 lines top border
        56 border lines bottom border
        48 pixels on each side horizontal border
    */
    const int top_decode_line = zx->top_border_scanlines - 32;
    const int btm_decode_line = zx->top_border_scanlines + 192 + 32;
    if ((zx->scanline_y >= top_decode_line) && (zx->scanline_y < btm_decode_line)) {
        const uint16_t y = zx->scanline_y - top_decode_line;
        uint32_t* dst = &rgba8_buffer[y * ZX_DISP_WIDTH];
        const uint8_t* vidmem_bank = zx->ram[zx->display_ram_bank];
        const bool blink = 0 != (zx->blink_counter & 0x10);
        uint32_t fg, bg;
        if ((y < 32) || (y >= 224)) {
            /* upper/lower border */
            for (int x = 0; x < ZX_DISP_WIDTH; x++) {
                *dst++ = zx->border_color;
            }
        }
        else {
            /* compute video memory Y offset (inside 256x192 area)
                this is how the 16-bit video memory address is computed
                from X and Y coordinates:
                | 0| 1| 0|Y7|Y6|Y2|Y1|Y0|Y5|Y4|Y3|X4|X3|X2|X1|X0|
            */
            const uint16_t yy = y-32;
            const uint16_t y_offset = ((yy & 0xC0)<<5) | ((yy & 0x07)<<8) | ((yy & 0x38)<<2);

            /* left border */
            for (int x = 0; x < (4*8); x++) {
                *dst++ = zx->border_color;
            }

            /* valid 256x192 vidmem area */
            for (uint16_t x = 0; x < 32; x++) {
                const uint16_t pix_offset = y_offset | x;
                const uint16_t clr_offset = 0x1800 + (((yy & ~0x7)<<2) | x);

                /* pixel mask and color attribute bytes */
                const uint8_t pix = vidmem_bank[pix_offset];
                const uint8_t clr = vidmem_bank[clr_offset];

                /* foreground and background color */
                if ((clr & (1<<7)) && blink) {
                    fg = zx_palette[(clr>>3) & 7];
                    bg = zx_palette[clr & 7];
                }
                else {
                    fg = zx_palette[clr & 7];
                    bg = zx_palette[(clr>>3) & 7];
                }
                if (0 == (clr & (1<<6))) {
                    // standard brightness
                    fg &= 0xFFD7D7D7;
                    bg &= 0xFFD7D7D7;
                }
                for (int px = 7; px >=0; px--) {
                    *dst++ = pix & (1<<px) ? fg : bg;
                }
            }

            /* right border */
            for (int x = 0; x < (4*8); x++) {
                *dst++ = zx->border_color;
            }
        }
    }

    if (zx->scanline_y++ >= zx->frame_scan_lines) {
        /* start new frame, request vblank interrupt */
        zx->scanline_y = 0;
        zx->blink_counter++;
        return true;
    }
    else {
        return false;
    }
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

bool zx_load_z80(zx_t* zx, const uint8_t* ptr, uint32_t num_bytes) {
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
            if (zx->type != ZX_TYPE_48K) {
                return false;
            }
        }
        else {
            if (zx->type != ZX_TYPE_128K) {
                return false;
            }
        }
    }
    else {
        if (zx->type != ZX_TYPE_48K) {
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
            if ((zx->type == ZX_TYPE_48K) && (page_index == 5)) {
                page_index = 0;
            }
            if ((page_index < 0) || (page_index > 7)) {
                page_index = -1;
            }
        }
        uint8_t* dst_ptr;
        if (-1 == page_index) {
            dst_ptr = zx->junk;
        }
        else {
            dst_ptr = zx->ram[page_index];
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
    z80_reset(&zx->cpu);
    z80_set_a(&zx->cpu, hdr->A); z80_set_f(&zx->cpu, hdr->F);
    z80_set_b(&zx->cpu, hdr->B); z80_set_c(&zx->cpu, hdr->C);
    z80_set_d(&zx->cpu, hdr->D); z80_set_e(&zx->cpu, hdr->E);
    z80_set_h(&zx->cpu, hdr->H); z80_set_l(&zx->cpu, hdr->L);
    z80_set_ix(&zx->cpu, hdr->IX_h<<8|hdr->IX_l);
    z80_set_iy(&zx->cpu, hdr->IY_h<<8|hdr->IY_l);
    z80_set_af_(&zx->cpu, hdr->A_<<8|hdr->F_);
    z80_set_bc_(&zx->cpu, hdr->B_<<8|hdr->C_);
    z80_set_de_(&zx->cpu, hdr->D_<<8|hdr->E_);
    z80_set_hl_(&zx->cpu, hdr->H_<<8|hdr->L_);
    z80_set_sp(&zx->cpu, hdr->SP_h<<8|hdr->SP_l);
    z80_set_i(&zx->cpu, hdr->I);
    z80_set_r(&zx->cpu, (hdr->R & 0x7F) | ((hdr->flags0 & 1)<<7));
    z80_set_iff2(&zx->cpu, hdr->IFF2 != 0);
    z80_set_ei_pending(&zx->cpu, hdr->EI != 0);
    if (hdr->flags1 != 0xFF) {
        z80_set_im(&zx->cpu, hdr->flags1 & 3);
    }
    else {
        z80_set_im(&zx->cpu, 1);
    }
    if (ext_hdr) {
        z80_set_pc(&zx->cpu, ext_hdr->PC_h<<8|ext_hdr->PC_l);
        if (zx->type == ZX_TYPE_128K) {
            for (int i = 0; i < 16; i++) {
                /* latch AY-3-8912 register address */
                ay38910_iorq(&zx->ay, AY38910_BDIR|AY38910_BC1|(i<<16));
                /* write AY-3-8912 register value */
                ay38910_iorq(&zx->ay, AY38910_BDIR|(ext_hdr->audio[i]<<16));
            }
        }
        /* simulate an out of port 0xFFFD and 0x7FFD */
        uint64_t pins = Z80_IORQ|Z80_WR;
        Z80_SET_ADDR(pins, 0xFFFD);
        Z80_SET_DATA(pins, ext_hdr->out_fffd);
        zx_cpu_tick(4, pins, zx);
        Z80_SET_ADDR(pins, 0x7FFD);
        Z80_SET_DATA(pins, ext_hdr->out_7ffd);
        zx_cpu_tick(4, pins, zx);
    }
    else {
        z80_set_pc(&zx->cpu, hdr->PC_h<<8|hdr->PC_l);
    }
    zx->border_color = zx_palette[(hdr->flags0>>1) & 7] & 0xFFD7D7D7;
    return true;
}

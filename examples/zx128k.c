/*
    zx128k.c
    ZX Spectrum 128 emulator.
    - the beeper and AY-3-8192 are both emulated, but there's no sound output
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
#include "roms/zx128k-roms.h"
#include "common/gfx.h"
#include "common/sound.h"
#include "common/clock.h"

/* ZX Spectrum state and callbacks */
#define ZX128K_FREQ (3546894)
#define ZX128K_DISP_WIDTH (320)
#define ZX128K_DISP_HEIGHT (256)
#define ZX128K_SCANLINES (311)
#define ZX128K_TOP_BORDER_SCANLINES (63)
#define ZX128K_SCANLINE_PERIOD (228)

typedef struct {
    z80_t cpu;
    beeper_t beeper;
    ay38910_t ay;
    kbd_t kbd;
    mem_t mem;
    bool memory_paging_disabled;
    uint32_t tick_count;
    uint8_t last_fe_out;            // last out value to 0xFE port */
    uint8_t blink_counter;          // incremented on each vblank
    int scanline_counter;
    int scanline_y;
    uint32_t display_ram_bank;
    uint32_t border_color;
    uint8_t ram[8][0x4000];
} zx128k_t;
zx128k_t zx;

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

void zx_init(void);
uint64_t zx_cpu_tick(int num_ticks, uint64_t pins);
bool zx_decode_scanline(void);

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * ZX128K_DISP_WIDTH,
        .height = 2 * ZX128K_DISP_HEIGHT,
        .window_title = "ZX Spectrum 128",
        .ios_keyboard_resizes_canvas = true
    };
}

/* one-time application init */
void app_init() {
    gfx_init(ZX128K_DISP_WIDTH, ZX128K_DISP_HEIGHT, 1, 1);
    sound_init();
    clock_init(ZX128K_FREQ);
    zx_init();
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    clock_ticks_executed(z80_exec(&zx.cpu, clock_ticks_to_run()));
    kbd_update(&zx.kbd);
    gfx_draw();
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
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
                case SAPP_KEYCODE_SPACE:        c = 0x20; break;
                case SAPP_KEYCODE_LEFT:         c = 0x08; break;
                case SAPP_KEYCODE_RIGHT:        c = 0x09; break;
                case SAPP_KEYCODE_DOWN:         c = 0x0A; break;
                case SAPP_KEYCODE_UP:           c = 0x0B; break;
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
void zx_init() {
    zx.border_color = 0xFF000000;
    zx.display_ram_bank = 5;
    zx.scanline_counter = ZX128K_SCANLINE_PERIOD;

    z80_init(&zx.cpu, zx_cpu_tick);
    beeper_init(&zx.beeper, ZX128K_FREQ, sound_sample_rate(), 0.5f);
    ay38910_init(&zx.ay, &(ay38910_desc_t){
        .type = AY38910_TYPE_8912,
        .tick_hz = ZX128K_FREQ/2,
        .sound_hz = sound_sample_rate(),
        .magnitude = 0.5
    });
    zx.cpu.state.PC = 0x0000;

    /* initial memory map */
    mem_map_ram(&zx.mem, 0, 0x4000, 0x4000, zx.ram[5]);
    mem_map_ram(&zx.mem, 0, 0x8000, 0x4000, zx.ram[2]);
    mem_map_ram(&zx.mem, 0, 0xC000, 0x4000, zx.ram[0]);
    mem_map_rom(&zx.mem, 0, 0x0000, 0x4000, dump_amstrad_zx128k_0);

    /* setup keyboard matrix */
    kbd_init(&zx.kbd, 1);
    /* caps-shift is column 0, line 0 */
    kbd_register_modifier(&zx.kbd, 0, 0, 0);
    /* sym-shift is column 7, line 1 */
    kbd_register_modifier(&zx.kbd, 1, 7, 1);
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
                    kbd_register_key(&zx.kbd, c, col, line, (layer>0) ? (1<<(layer-1)) : 0);
                }
            }
        }
    }

    /* special keys */
    kbd_register_key(&zx.kbd, ' ', 7, 0, 0);  // Space
    kbd_register_key(&zx.kbd, 0x0F, 7, 1, 0); // SymShift
    kbd_register_key(&zx.kbd, 0x08, 3, 4, 1); // Cursor Left (Shift+5)
    kbd_register_key(&zx.kbd, 0x0A, 4, 4, 1); // Cursor Down (Shift+6)
    kbd_register_key(&zx.kbd, 0x0B, 4, 3, 1); // Cursor Up (Shift+7)
    kbd_register_key(&zx.kbd, 0x09, 4, 2, 1); // Cursor Right (Shift+8)
    kbd_register_key(&zx.kbd, 0x07, 3, 0, 1); // Edit (Shift+1)
    kbd_register_key(&zx.kbd, 0x0C, 4, 0, 1); // Delete (Shift+0)
    kbd_register_key(&zx.kbd, 0x0D, 6, 0, 0); // Enter
}

/* the CPU tick callback */
uint64_t zx_cpu_tick(int num_ticks, uint64_t pins) {
    /* video decoding and vblank interrupt */
    zx.scanline_counter -= num_ticks;
    if (zx.scanline_counter <= 0) {
        zx.scanline_counter += ZX128K_SCANLINE_PERIOD;
        // decode next video scanline
        if (zx_decode_scanline()) {
            // request vblank interrupt
            pins |= Z80_INT;
        }
    }

    /* tick audio systems */
    for (int i = 0; i < num_ticks; i++) {
        zx.tick_count++;
        bool sample_ready = beeper_tick(&zx.beeper);
        /* the AY-3-8912 chip runs at half CPU frequency */
        if (zx.tick_count & 1) {
            ay38910_tick(&zx.ay);
        }
        if (sample_ready) {
            sound_push(zx.beeper.sample + zx.ay.sample);
        }
    }

    /* memory and IO requests */
    if (pins & Z80_MREQ) {
        /* a memory request machine cycle
            FIXME: 'contended memory' accesses should inject wait states
        */
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem_rd(&zx.mem, addr));
        }
        else if (pins & Z80_WR) {
            mem_wr(&zx.mem, addr, Z80_GET_DATA(pins));
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
                if (zx.last_fe_out & (1<<3|1<<4)) {
                    data |= (1<<6);
                }
                /* keyboard matrix bits are encoded in the upper 8 bit of the port address */
                uint16_t column_mask = (~(Z80_GET_ADDR(pins)>>8)) & 0x00FF;
                const uint16_t kbd_lines = kbd_test_lines(&zx.kbd, column_mask);
                data |= (~kbd_lines) & 0x1F;
                Z80_SET_DATA(pins, data);
            }
            else if ((pins & (Z80_A7|Z80_A6|Z80_A5)) == 0) {
                /* FIXME: Kempston Joystick (........000.....) */
            }
            else {
                /* read from AY-3-8912 (11............0.) */
                if ((pins & (Z80_A15|Z80_A14|Z80_A1)) == (Z80_A15|Z80_A14)) {
                    pins = ay38910_iorq(&zx.ay, AY38910_BC1|pins) & Z80_PIN_MASK;
                }
            }
        }
        else if (pins & Z80_WR) {
            // an IO write
            const uint8_t data = Z80_GET_DATA(pins);
            if ((pins & Z80_A0) == 0) {
                // Spectrum ULA (...............0)
                // "every even IO port addresses the ULA but to avoid
                // problems with other I/O devices, only FE should be used"
                zx.border_color = zx_palette[data & 7] & 0xFFD7D7D7;
                // FIXME:
                //      bit 3: MIC output (CAS SAVE, 0=On, 1=Off)
                //      bit 4: Beep output (ULA sound, 0=Off, 1=On)
                zx.last_fe_out = data;
                beeper_set(&zx.beeper, 0 != (data & (1<<4)));
            }
            else {
                /* Spectrum 128 memory control (0.............0.)
                    http://8bit.yarek.pl/computer/zx.128/
                */
                if ((pins & (Z80_A15|Z80_A1)) == 0) {
                    if (!zx.memory_paging_disabled) {
                        // bit 3 defines the video scanout memory bank (5 or 7)
                        zx.display_ram_bank = (data & (1<<3)) ? 7 : 5;
                        // only last memory bank is mappable
                        mem_map_ram(&zx.mem, 0, 0xC000, 0x4000, zx.ram[data & 0x7]);

                        // ROM0 or ROM1
                        if (data & (1<<4)) {
                            // bit 4 set: ROM1
                            mem_map_rom(&zx.mem, 0, 0x0000, 0x4000, dump_amstrad_zx128k_1);
                        }
                        else {
                            // bit 4 clear: ROM0
                            mem_map_rom(&zx.mem, 0, 0x0000, 0x4000, dump_amstrad_zx128k_0);
                        }
                    }
                    if (data & (1<<5)) {
                        /* bit 5 prevents further changes to memory pages
                            until computer is reset, this is used when switching
                            to the 48k ROM
                        */
                        zx.memory_paging_disabled = true;
                    }
                }
                else if ((pins & (Z80_A15|Z80_A14|Z80_A1)) == (Z80_A15|Z80_A14)) {
                    /* select AY-3-8912 register (11............0.) */
                    ay38910_iorq(&zx.ay, AY38910_BDIR|AY38910_BC1|pins);
                }
                else if ((pins & (Z80_A15|Z80_A14|Z80_A1)) == Z80_A15) {
                    /* write to AY-3-8912 (10............0.) */
                    ay38910_iorq(&zx.ay, AY38910_BDIR|pins);
                }
            }
        }
    }
    return pins;
}

bool zx_decode_scanline() {
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
    const int top_decode_line = ZX128K_TOP_BORDER_SCANLINES - 32;
    const int btm_decode_line = ZX128K_TOP_BORDER_SCANLINES + 192 + 32;
    if ((zx.scanline_y >= top_decode_line) && (zx.scanline_y < btm_decode_line)) {
        const uint16_t y = zx.scanline_y - top_decode_line;
        uint32_t* dst = &rgba8_buffer[y * ZX128K_DISP_WIDTH];
        const uint8_t* vidmem_bank = zx.ram[zx.display_ram_bank];
        const bool blink = 0 != (zx.blink_counter & 0x10);
        uint32_t fg, bg;
        if ((y < 32) || (y >= 224)) {
            /* upper/lower border */
            for (int x = 0; x < ZX128K_DISP_WIDTH; x++) {
                *dst++ = zx.border_color;
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
                *dst++ = zx.border_color;
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
                *dst++ = zx.border_color;
            }
        }
    }

    if (zx.scanline_y++ >= ZX128K_SCANLINES) {
        /* start new frame, request vblank interrupt */
        zx.scanline_y = 0;
        zx.blink_counter++;
        return true;
    }
    else {
        return false;
    }
}

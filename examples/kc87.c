//------------------------------------------------------------------------------
//  kc87.c
//
//  Wiring diagram: http://www.sax.de/~zander/kc/kcsch_1.pdf
//  Detailed Manual: http://www.sax.de/~zander/z9001/doku/z9_fub.pdf
//
//  not emulated: beeper sound, border color, 40x20 video mode
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_time.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/z80ctc.h"
#include "chips/kbd.h"
#include "roms/kc87-roms.h"
#include "common/gfx.h"
#include <ctype.h> /* isupper, islower, toupper, tolower */

/* KC87 emulator state and callbacks */
#define KC87_FREQ (2457600)
#define KC87_DISP_WIDTH (320)
#define KC87_DISP_HEIGHT (192)
z80_t cpu;
z80pio_t pio1;
z80pio_t pio2;
z80ctc_t ctc;
kbd_t kbd;
uint32_t blink_counter = 0;
bool blink_flip_flop = false;
uint64_t ctc_zcto2 = 0;
uint8_t mem[1<<16];
uint32_t overrun_ticks;
uint64_t last_time_stamp;
const uint32_t palette[8] = {
    0xFF000000,     // black
    0xFF0000FF,     // red
    0xFF00FF00,     // green
    0xFF00FFFF,     // yellow
    0xFFFF0000,     // blue
    0xFFFF00FF,     // purple
    0xFFFFFF00,     // cyan
    0xFFFFFFFF,     // white
};
void kc87_init(void);
uint64_t kc87_tick(int num, uint64_t pins);
uint8_t kc87_pio1_in(int port_id);
void kc87_pio1_out(int port_id, uint8_t data);
uint8_t kc87_pio2_in(int port_id);
void kc87_pio2_out(int port_id, uint8_t data);
void kc87_decode_vidmem(void);

/* xorshift randomness for memory initialization */
uint32_t xorshift_state = 0x6D98302B;
uint32_t xorshift32() {
    uint32_t x = xorshift_state;
    x ^= x<<13; x ^= x>>17; x ^= x<<5;
    xorshift_state = x;
    return x;
}

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);
sapp_desc sokol_main() {
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * KC87_DISP_WIDTH,
        .height = 2 * KC87_DISP_HEIGHT,
        .window_title = "KC87"
    };
}

/* one-time application init */
void app_init() {
    gfx_init(KC87_DISP_WIDTH, KC87_DISP_HEIGHT);
    kc87_init();
    last_time_stamp = stm_now();
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    double frame_time = stm_sec(stm_laptime(&last_time_stamp));
    uint32_t ticks_to_run = (uint32_t) ((KC87_FREQ * frame_time) - overrun_ticks);
    uint32_t ticks_executed = z80_exec(&cpu, ticks_to_run);
    assert(ticks_executed >= ticks_to_run);
    overrun_ticks = ticks_executed - ticks_to_run;
    kbd_update(&kbd);
    kc87_decode_vidmem();
    gfx_draw();
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if (c < KBD_MAX_KEYS) {
                /* need to invert case (unshifted is upper caps, shifted is lower caps */
                if (isupper(c)) {
                    c = tolower(c);
                }
                else if (islower(c)) {
                    c = toupper(c);
                }
                kbd_key_down(&kbd, c);
                kbd_key_up(&kbd, c);
            }
            /* keyboard matrix lines are directly connected to the PIO2's Port B */
            z80pio_write_port(&pio2, Z80PIO_PORT_B, ~kbd_scan_lines(&kbd));
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            c = (int) event->key_code;
            switch (c) {
                case SAPP_KEYCODE_ENTER:    c = 0x0D; break;
                case SAPP_KEYCODE_RIGHT:    c = 0x09; break;
                case SAPP_KEYCODE_LEFT:     c = 0x08; break;
                case SAPP_KEYCODE_DOWN:     c = 0x0A; break;
                case SAPP_KEYCODE_UP:       c = 0x0B; break;
                case SAPP_KEYCODE_ESCAPE:   c = (event->modifiers & SAPP_MODIFIER_SHIFT)? 0x1B: 0x03; break;
                case SAPP_KEYCODE_INSERT:   c = 0x1A; break;
                case SAPP_KEYCODE_HOME:     c = 0x19; break;
                default:                    c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    kbd_key_down(&kbd, c);
                }
                else {
                    kbd_key_up(&kbd, c);
                }
                /* keyboard matrix lines are directly connected to the PIO2's Port B */
                z80pio_write_port(&pio2, Z80PIO_PORT_B, ~kbd_scan_lines(&kbd));
            }
            break;
        default:
            break;
    }
}

/* application cleanup callback */
void app_cleanup() {
    gfx_shutdown();
}

/* KC87 emulator initialization */
void kc87_init() {
    /* initialize CPU, PIOs and CTC */
    z80_init(&cpu, kc87_tick);
    z80pio_init(&pio1, kc87_pio1_in, kc87_pio1_out);
    z80pio_init(&pio2, kc87_pio2_in, kc87_pio2_out);
    z80ctc_init(&ctc);

    /* setup keyboard matrix, keep keys pressed for N frames to give
       the scan-out routine enough time
    */
    kbd_init(&kbd, 3);
    /* shift key is column 0, line 7 */
    kbd_register_modifier(&kbd, 0, 0, 7);
    /* register alpha-numeric keys */
    const char* keymap =
        /* unshifted keys */
        "01234567"
        "89:;,=.?"
        "@ABCDEFG"
        "HIJKLMNO"
        "PQRSTUVW"
        "XYZ   ^ "
        "        "
        "        "
        /* shifted keys */
        "_!\"#$%&'"
        "()*+<->/"
        " abcdefg"
        "hijklmno"
        "pqrstuvw"
        "xyz     "
        "        "
        "        ";
    for (int shift = 0; shift < 2; shift++) {
        for (int line = 0; line < 8; line++) {
            for (int col = 0; col < 8; col++) {
                int c = keymap[shift*64 + line*8 + col];
                if (c != 0x20) {
                    kbd_register_key(&kbd, c, col, line, shift?(1<<0):0);
                }
            }
        }
    }
    /* special keys */
    kbd_register_key(&kbd, 0x03, 6, 6, 0);      /* stop (Esc) */
    kbd_register_key(&kbd, 0x08, 0, 6, 0);      /* cursor left */
    kbd_register_key(&kbd, 0x09, 1, 6, 0);      /* cursor right */
    kbd_register_key(&kbd, 0x0A, 2, 6, 0);      /* cursor up */
    kbd_register_key(&kbd, 0x0B, 3, 6, 0);      /* cursor down */
    kbd_register_key(&kbd, 0x0D, 5, 6, 0);      /* enter */
    kbd_register_key(&kbd, 0x13, 4, 5, 0);      /* pause */
    kbd_register_key(&kbd, 0x14, 1, 7, 0);      /* color */
    kbd_register_key(&kbd, 0x19, 3, 5, 0);      /* home */
    kbd_register_key(&kbd, 0x1A, 5, 5, 0);      /* insert */
    kbd_register_key(&kbd, 0x1B, 4, 6, 0);      /* esc (Shift+Esc) */
    kbd_register_key(&kbd, 0x1C, 4, 7, 0);      /* list */
    kbd_register_key(&kbd, 0x1D, 5, 7, 0);      /* run */
    kbd_register_key(&kbd, 0x20, 7, 6, 0);      /* space */

    /* fill memory with randonmess */
    for (int i = 0; i < (int) sizeof(mem);) {
        uint32_t r = xorshift32();
        mem[i++] = r>>24;
        mem[i++] = r>>16;
        mem[i++] = r>>8;
        mem[i++] = r;
    }
    /* 8 KBytes BASIC ROM starting at C000 */
    assert(sizeof(dump_z9001_basic) == 0x2000);
    memcpy(&mem[0xC000], dump_z9001_basic, sizeof(dump_z9001_basic));
    /* 8 KByte OS ROM starting at E000, leave a hole at E800..EFFF for vidmem */
    assert(sizeof(dump_kc87_os_2) == 0x2000);
    memcpy(&mem[0xE000], dump_kc87_os_2, 0x800);
    memcpy(&mem[0xF000], dump_kc87_os_2+0x1000, 0x1000);

    /* execution starts at 0xF000 */
    cpu.state.PC = 0xF000;
}

/* the CPU tick callback performs memory and I/O reads/writes */
uint64_t kc87_tick(int num_ticks, uint64_t pins) {
    /* tick the CTC channels, the CTC channel 2 output signal ZCTO2 is connected
       to CTC channel 3 input signal CLKTRG3 to form a timer cascade
       which drives the system clock, store the state of ZCTO2 for the
       next tick
    */
    pins |= ctc_zcto2;
    for (int i = 0; i < num_ticks; i++) {
        if (pins & Z80CTC_ZCTO2) { pins |= Z80CTC_CLKTRG3; }
        else                     { pins &= ~Z80CTC_CLKTRG3; }
        pins = z80ctc_tick(&ctc, pins);

    }
    ctc_zcto2 = (pins & Z80CTC_ZCTO2);

    /* the blink flip flop is controlled by a 'bisync' video signal
       (I guess that means it triggers at half PAL frequency: 25Hz),
       going into a binary counter, bit 4 of the counter is connected
       to the blink flip flop.
    */
    for (int i = 0; i < num_ticks; i++) {
        if (0 >= blink_counter--) {
            blink_counter = (KC87_FREQ * 8) / 25;
            blink_flip_flop = !blink_flip_flop;
        }
    }

    /* memory and IO requests */
    if (pins & Z80_MREQ) {
        /* a memory request machine cycle */
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            /* read memory byte */
            Z80_SET_DATA(pins, mem[addr]);
        }
        else if (pins & Z80_WR) {
            /* write memory byte, don't overwrite ROM */
            if ((addr < 0xC000) || ((addr >= 0xE800) && (addr < 0xF000))) {
                mem[addr] = Z80_GET_DATA(pins);
            }
        }
    }
    else if (pins & Z80_IORQ) {
        /* an IO request machine cycle */

        /* check if any of the PIO/CTC chips is enabled */
        /* address line 7 must be on, 6 must be off, IORQ on, M1 off */
        const bool chip_enable = (pins & (Z80_IORQ|Z80_M1|Z80_A7|Z80_A6)) == (Z80_IORQ|Z80_A7);
        const int chip_select = (pins & (Z80_A5|Z80_A4|Z80_A3))>>3;

        pins = pins & Z80_PIN_MASK;
        if (chip_enable) {
            switch (chip_select) {
                /* IO request on CTC? */
                case 0:
                    /* CTC is mapped to ports 0x80 to 0x87 (each port is mapped twice) */
                    pins |= Z80CTC_CE;
                    if (pins & Z80_A0) { pins |= Z80CTC_CS0; };
                    if (pins & Z80_A1) { pins |= Z80CTC_CS1; };
                    pins = z80ctc_iorq(&ctc, pins) & Z80_PIN_MASK;
                    break;
                /* IO request on PIO1? */
                case 1:
                    /* PIO1 is mapped to ports 0x88 to 0x8F (each port is mapped twice) */
                    pins |= Z80PIO_CE;
                    if (pins & Z80_A0) { pins |= Z80PIO_BASEL; }
                    if (pins & Z80_A1) { pins |= Z80PIO_CDSEL; }
                    pins = z80pio_iorq(&pio1, pins) & Z80_PIN_MASK;
                    break;
                /* IO request on PIO2? */
                case 2:
                    /* PIO2 is mapped to ports 0x90 to 0x97 (each port is mapped twice) */
                    pins |= Z80PIO_CE;
                    if (pins & Z80_A0) { pins |= Z80PIO_BASEL; }
                    if (pins & Z80_A1) { pins |= Z80PIO_CDSEL; }
                    pins = z80pio_iorq(&pio2, pins) & Z80_PIN_MASK;
            }
        }
    }

    /* handle interrupt requests by PIOs and CTCs, the interrupt priority
       is PIO1>PIO2>CTC, the interrupt handling functions must be called
       in this order
    */
    Z80_DAISYCHAIN_BEGIN(pins)
    {
        pins = z80pio_int(&pio1, pins);
        pins = z80pio_int(&pio2, pins);
        pins = z80ctc_int(&ctc, pins);
    }
    Z80_DAISYCHAIN_END(pins);
    return (pins & Z80_PIN_MASK);
}

uint8_t kc87_pio1_in(int port_id) {
    return 0x00;
}

void kc87_pio1_out(int port_id, uint8_t data) {
    if (Z80PIO_PORT_A == port_id) {
        /*
            PIO1-A bits:
            0..1:    unused
            2:       display mode (0: 24 lines, 1: 20 lines)
            3..5:    border color
            6:       graphics LED on keyboard (0: off)
            7:       enable audio output (1: enabled)
        */
        // FIXME: border_color = (data>>3) & 7;
    }
    else {
        /* PIO1-B is reserved for external user devices */
    }
}

/*
    PIO2 is reserved for keyboard input which works like this:

    FIXME: describe keyboard input
*/
uint8_t kc87_pio2_in(int port_id) {
    if (Z80PIO_PORT_A == port_id) {
        /* return keyboard matrix column bits for requested line bits */
        uint8_t columns = (uint8_t) kbd_scan_columns(&kbd);
        return ~columns;
    }
    else {
        /* return keyboard matrix line bits for requested column bits */
        uint8_t lines = (uint8_t) kbd_scan_lines(&kbd);
        return ~lines;
    }
}

void kc87_pio2_out(int port_id, uint8_t data) {
    if (Z80PIO_PORT_A == port_id) {
        kbd_set_active_columns(&kbd, ~data);
    }
    else {
        kbd_set_active_lines(&kbd, ~data);
    }
}

/* decode the KC87 40x24 framebuffer to a linear 320x192 RGBA8 buffer */
void kc87_decode_vidmem() {
    /* FIXME: there's also a 40x20 video mode */
    uint32_t* dst = rgba8_buffer;
    const uint8_t* vidmem = &mem[0xEC00];     /* 1 KB ASCII buffer at EC00 */
    const uint8_t* colmem = &mem[0xE800];     /* 1 KB color buffer at E800 */
    const uint8_t* font = dump_kc87_font_2;
    int offset = 0;
    uint32_t fg, bg;
    for (int y = 0; y < 24; y++) {
        for (int py = 0; py < 8; py++) {
            for (int x = 0; x < 40; x++) {
                uint8_t chr = vidmem[offset+x];
                uint8_t pixels = font[(chr<<3)|py];
                uint8_t color = colmem[offset+x];
                if ((color & 0x80) && blink_flip_flop) {
                    /* blinking: swap back- and foreground color */
                    fg = palette[color&7];
                    bg = palette[(color>>4)&7];
                }
                else {
                    fg = palette[(color>>4)&7];
                    bg = palette[color&7];
                }
                for (int px = 7; px >= 0; px--) {
                    *dst++ = pixels & (1<<px) ? fg:bg;
                }
            }
        }
        offset += 40;
    }
}

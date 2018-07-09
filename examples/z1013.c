/*
    z1013.c

    The Z1013 was a very simple East German home computer, basically
    just a Z80 CPU connected to some memory, and a PIO connected to
    a keyboard matrix. It's easy to emulate because the system didn't
    use any interrupts, and only simple PIO IN/OUT is required to
    scan the keyboard matrix.

    It had a 32x32 monochrome ASCII framebuffer starting at EC00,
    and a 2 KByte operating system ROM starting at F000.

    No cassette-tape / beeper sound emulated!

    I have added a pre-loaded KC-BASIC interpreter:

    Start the BASIC interpreter with 'J 300', return to OS with 'BYE'.

    Enter BASIC editing mode with 'AUTO', leave by pressing Esc.
*/
#include "sokol_app.h"
#include "sokol_time.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/kbd.h"
#include "roms/z1013-roms.h"
#include "common/gfx.h"
#include <ctype.h> /* isupper, islower, toupper, tolower */

/* Z1013 emulator state and callbacks */
#define Z1013_FREQ (2000000)
#define Z1013_DISP_WIDTH (256)
#define Z1013_DISP_HEIGHT (256)

typedef struct {
    z80_t cpu;
    z80pio_t pio;
    kbd_t kbd;
    uint8_t kbd_request_column;
    bool kbd_request_line_hilo;
    uint8_t mem[1<<16];
} z1013_t;
z1013_t z1013;

void z1013_init(void);
uint64_t z1013_tick(int num, uint64_t pins);
uint8_t z1013_pio_in(int port_id);
void z1013_pio_out(int port_id, uint8_t data);
void z1013_decode_vidmem(void);

uint32_t overrun_ticks;
uint64_t last_time_stamp;

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
        .width = 2 * Z1013_DISP_WIDTH,
        .height = 2 * Z1013_DISP_HEIGHT,
        .window_title = "Z1013",
        .ios_keyboard_resizes_canvas = true
    };
}

/* one-time application init */
void app_init(void) {
    gfx_init(Z1013_DISP_WIDTH, Z1013_DISP_HEIGHT, 1, 1);
    z1013_init();
    last_time_stamp = stm_now();
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame(void) {
    double frame_time = stm_sec(stm_laptime(&last_time_stamp));
    /* skip long pauses when the app was suspended */
    if (frame_time > 0.1) {
        frame_time = 0.1;
    }
    /* number of 2MHz ticks in host frame */
    uint32_t ticks_to_run = (uint32_t) ((Z1013_FREQ * frame_time) - overrun_ticks);
    uint32_t ticks_executed = z80_exec(&z1013.cpu, ticks_to_run);
    assert(ticks_executed >= ticks_to_run);
    overrun_ticks = ticks_executed - ticks_to_run;
    kbd_update(&z1013.kbd);
    z1013_decode_vidmem();
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
                kbd_key_down(&z1013.kbd, c);
                kbd_key_up(&z1013.kbd, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_ENTER:    c = 0x0D; break;
                case SAPP_KEYCODE_RIGHT:    c = 0x09; break;
                case SAPP_KEYCODE_LEFT:     c = 0x08; break;
                case SAPP_KEYCODE_DOWN:     c = 0x0A; break;
                case SAPP_KEYCODE_UP:       c = 0x0B; break;
                case SAPP_KEYCODE_ESCAPE:   c = 0x03; break;
                default:                    c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    kbd_key_down(&z1013.kbd, c);
                }
                else {
                    kbd_key_up(&z1013.kbd, c);
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
void app_cleanup(void) {
    gfx_shutdown();
}

/* Z1013 emulator initialization */
void z1013_init(void) {
    /* initialize the Z80 CPU and PIO */
    z80_init(&z1013.cpu, z1013_tick);
    z80pio_init(&z1013.pio, z1013_pio_in, z1013_pio_out);

    /* setup the 8x8 keyboard matrix (see http://www.z1013.de/images/21.gif)
       keep keys pressed for at least 2 frames to give the
       Z1013 enough time to scan the keyboard
    */
    kbd_init(&z1013.kbd, 2);
    /* shift key is column 7, line 6 */
    const int shift = 0, shift_mask = (1<<shift);
    kbd_register_modifier(&z1013.kbd, shift, 7, 6);
    /* ctrl key is column 6, line 5 */
    const int ctrl = 1, ctrl_mask = (1<<ctrl);
    kbd_register_modifier(&z1013.kbd, ctrl, 6, 5);
    /* alpha-numeric keys */
    const char* keymap =
        /* unshifted keys */
        "13579-  QETUO@  ADGJL*  YCBM.^  24680[  WRZIP]  SFHK+\\  XVN,/_  "
        /* shift layer */
        "!#%')=  qetuo`  adgjl:  ycbm>~  \"$&( {  wrzip}  sfhk;|  xvn<?   ";
    for (int layer = 0; layer < 2; layer++) {
        for (int line = 0; line < 8; line++) {
            for (int col = 0; col < 8; col++) {
                int c = keymap[layer*64 + line*8 + col];
                if (c != 0x20) {
                    kbd_register_key(&z1013.kbd, c, col, line, layer?shift_mask:0);
                }
            }
        }
    }
    /* special keys */
    kbd_register_key(&z1013.kbd, ' ',  6, 4, 0);  /* space */
    kbd_register_key(&z1013.kbd, 0x08, 6, 2, 0);  /* cursor left */
    kbd_register_key(&z1013.kbd, 0x09, 6, 3, 0);  /* cursor right */
    kbd_register_key(&z1013.kbd, 0x0A, 6, 7, 0);  /* cursor down */
    kbd_register_key(&z1013.kbd, 0x0B, 6, 6, 0);  /* cursor up */
    kbd_register_key(&z1013.kbd, 0x0D, 6, 1, 0);  /* enter */
    kbd_register_key(&z1013.kbd, 0x03, 1, 3, ctrl_mask); /* map Esc to Ctrl+C (STOP/BREAK) */

    /* 2 KByte system rom starting at 0xF000 */
    assert(sizeof(dump_z1013_mon_a2) == 2048);
    memcpy(&z1013.mem[0xF000], dump_z1013_mon_a2, sizeof(dump_z1013_mon_a2));

    /* copy BASIC interpreter to 0x0100, skip first 0x20 bytes .z80 file format header */
    assert(0x0100 + sizeof(dump_kc_basic) < 0xF000);
    memcpy(&z1013.mem[0x0100], dump_kc_basic+0x20, sizeof(dump_kc_basic)-0x20);

    /* execution starts at 0xF000 */
    z1013.cpu.state.PC = 0xF000;
}

/* the CPU tick function needs to perform memory and I/O reads/writes */
uint64_t z1013_tick(int num_ticks, uint64_t pins) {
    if (pins & Z80_MREQ) {
        /* a memory request */
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            /* read memory byte */
            Z80_SET_DATA(pins, z1013.mem[addr]);
        }
        else if (pins & Z80_WR) {
            /* write memory byte, don't overwrite ROM */
            if (addr < 0xF000) {
                z1013.mem[addr] = Z80_GET_DATA(pins);
            }
        }
    }
    else if (pins & Z80_IORQ) {
        /* an I/O request */
        /*
            The PIO Chip-Enable pin (Z80PIO_CE) is connected to output pin 0 of
            a MH7442 BCD-to-Decimal decoder (looks like this is a Czech
            clone of a SN7442). The lower 3 input pins of the MH7442
            are connected to address bus pins A2..A4, and the 4th input
            pin is connected to IORQ. This means the PIO is enabled when
            the CPU IORQ pin is low (active), and address bus bits 2..4 are
            off. This puts the PIO at the lowest 4 addresses of an 32-entry
            address space (so the PIO should be visible at port number
            0..4, but also at 32..35, 64..67 and so on).

            The PIO Control/Data select pin (Z80PIO_CDSEL) is connected to
            address bus pin A0. This means even addresses select a PIO data
            operation, and odd addresses select a PIO control operation.

            The PIO port A/B select pin (Z80PIO_BASEL) is connected to address
            bus pin A1. This means the lower 2 port numbers address the PIO
            port A, and the upper 2 port numbers address the PIO port B.

            The keyboard matrix columns are connected to another MH7442
            BCD-to-Decimal converter, this converts a hardware latch at port
            address 8 which stores a keyboard matrix column number from the CPU
            to the column lines. The operating system scans the keyboard by
            writing the numbers 0..7 to this latch, which is then converted
            by the MH7442 to light up the keyboard matrix column lines
            in that order. Next the CPU reads back the keyboard matrix lines
            in 2 steps of 4 bits each from PIO port B.
        */
        if ((pins & (Z80_A4|Z80_A3|Z80_A2)) == 0) {
            /* address bits A2..A4 are zero, this activates the PIO chip-select pin */
            uint64_t pio_pins = (pins & Z80_PIN_MASK) | Z80PIO_CE;
            /* address bit 0 selects data/ctrl */
            if (pio_pins & (1<<0)) pio_pins |= Z80PIO_CDSEL;
            /* address bit 1 selects port A/B */
            if (pio_pins & (1<<1)) pio_pins |= Z80PIO_BASEL;
            pins = z80pio_iorq(&z1013.pio, pio_pins) & Z80_PIN_MASK;
        }
        else if ((pins & (Z80_A3|Z80_WR)) == (Z80_A3|Z80_WR)) {
            /* port 8 is connected to a hardware latch to store the
               requested keyboard column for the next keyboard scan
            */
            z1013.kbd_request_column = Z80_GET_DATA(pins);
        }
    }
    /* there are no interrupts happening in a vanilla Z1013,
       so don't trigger the interrupt daisy chain
    */
    return pins;
}

/* PIO input callback, scan the upper or lower 4 lines of the keyboard matrix */
uint8_t z1013_pio_in(int port_id) {
    uint8_t data = 0;
    if (Z80PIO_PORT_A == port_id) {
        /* PIO port A is reserved for user devices */
        data = 0xFF;
    }
    else {
        /* port B is for cassette input (bit 7), and lower 4 bits for kbd matrix lines */
        uint16_t column_mask = (1<<z1013.kbd_request_column);
        uint16_t line_mask = kbd_test_lines(&z1013.kbd, column_mask);
        if (z1013.kbd_request_line_hilo) {
            line_mask >>= 4;
        }
        data = 0xF & ~(line_mask & 0xF);
    }
    return data;
}

/* the PIO output callback selects the upper or lower 4 lines for the next keyboard scan */
void z1013_pio_out(int port_id, uint8_t data) {
    if (Z80PIO_PORT_B == port_id) {
        /* bit 4 for 8x8 keyboard selects upper or lower 4 kbd matrix line bits */
        z1013.kbd_request_line_hilo = 0 != (data & (1<<4));
        /* bit 7 is cassette output, not emulated */
    }
}

/* decode the Z1013 32x32 ASCII framebuffer to a linear 256x256 RGBA8 buffer */
void z1013_decode_vidmem(void) {
    uint32_t* dst = rgba8_buffer;
    const uint8_t* src = &z1013.mem[0xEC00];   /* the 32x32 framebuffer starts at EC00 */
    const uint8_t* font = dump_z1013_font;
    for (int y = 0; y < 32; y++) {
        for (int py = 0; py < 8; py++) {
            for (int x = 0; x < 32; x++) {
                uint8_t chr = src[(y<<5) + x];
                uint8_t bits = font[(chr<<3)|py];
                for (int px = 7; px >=0; px--) {
                    *dst++ = bits & (1<<px) ? 0xFFFFFFFF : 0xFF000000;
                }
            }
        }
    }
}

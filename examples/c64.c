/*
    c64.c
    SID is emulated, but there's no sound output. No tape or disc emulation.
    The original is part of the YAKC emulator: https://github.com/floooh/yakc
*/
#include "sokol_app.h"
#include "sokol_time.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "roms/c64-roms.h"
#include "common/gfx.h"
#include <ctype.h> /* isupper, islower, toupper, tolower */

/* C64 (PAL) emulator state and callbacks */
#define C64_FREQ (985248)
#define C64_DISP_X (64)
#define C64_DISP_Y (24)
#define C64_DISP_WIDTH (392)
#define C64_DISP_HEIGHT (272)

typedef struct {
    m6502_t cpu;
    m6526_t cia_1;
    m6526_t cia_2;
    m6569_t vic;
    m6581_t sid;
    kbd_t kbd;                  // keyboard matrix
    mem_t mem_cpu;              // how the CPU sees memory
    mem_t mem_vic;              // how the VIC sees memory
    uint8_t cpu_port;           // last state of CPU port (for memory mapping)
    uint16_t vic_bank_select;   // upper 4 address bits from CIA-2 port A
    bool io_mapped;             // true when D000..DFFF is has IO area mapped in
    uint8_t color_ram[1024];    // special static color ram
    uint8_t ram[1<<16];         // general ram
} c64_t;
static c64_t c64;

static uint32_t overrun_ticks;
static uint64_t last_time_stamp;

static void c64_init(void);
static void c64_update_memory_map(void);
static uint64_t c64_cpu_tick(uint64_t pins);
static uint8_t c64_cpu_port_in();
static void c64_cpu_port_out(uint8_t data);
static void c64_cia1_out(int port_id, uint8_t data);
static uint8_t c64_cia1_in(int port_id);
static void c64_cia2_out(int port_id, uint8_t data);
static uint8_t c64_cia2_in(int port_id);
static uint16_t c64_vic_fetch(uint16_t addr);

/* sokol-app entry, configure application callbacks and window */
static void app_init(void);
static void app_frame(void);
static void app_input(const sapp_event*);
static void app_cleanup(void);

sapp_desc sokol_main() {
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * C64_DISP_WIDTH,
        .height = 2 * C64_DISP_HEIGHT,
        .window_title = "C64"
    };
}

/* one-time application init */
void app_init() {
    gfx_init(C64_DISP_WIDTH, C64_DISP_HEIGHT);
    c64_init();
    last_time_stamp = stm_now();
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    double frame_time = stm_sec(stm_laptime(&last_time_stamp));
    /* skip long pauses when the app was suspended */
    if (frame_time > 0.1) {
        frame_time = 0.1;
    }
    uint32_t ticks_to_run = (uint32_t) ((C64_FREQ * frame_time) - overrun_ticks);
    uint32_t ticks_executed = m6502_exec(&c64.cpu, ticks_to_run);
    assert(ticks_executed >= ticks_to_run);
    overrun_ticks = ticks_executed - ticks_to_run;
    kbd_update(&c64.kbd);
    gfx_draw();
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
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
                kbd_key_down(&c64.kbd, c);
                kbd_key_up(&c64.kbd, c);
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
                case SAPP_KEYCODE_BACKSPACE:    c = shift ? 0x0C : 0x01; break;
                case SAPP_KEYCODE_ESCAPE:       c = shift ? 0x13 : 0x03; break;
                case SAPP_KEYCODE_F1:           c = 0xF1; break;
                case SAPP_KEYCODE_F2:           c = 0xF2; break;
                case SAPP_KEYCODE_F3:           c = 0xF3; break;
                case SAPP_KEYCODE_F4:           c = 0xF4; break;
                case SAPP_KEYCODE_F5:           c = 0xF5; break;
                case SAPP_KEYCODE_F6:           c = 0xF6; break;
                case SAPP_KEYCODE_F7:           c = 0xF7; break;
                case SAPP_KEYCODE_F8:           c = 0xF8; break;
                default:                        c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    kbd_key_down(&c64.kbd, c);
                }
                else {
                    kbd_key_up(&c64.kbd, c);
                }
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

/* C64 emulator init */
void c64_init() {
    c64.cpu_port = 0xF7;        // for initial memory configuration
    c64.io_mapped = true;

    /* initialize the CPU */
    m6502_init(&c64.cpu, &(m6502_desc_t){
        .tick_cb = c64_cpu_tick,
        .in_cb = c64_cpu_port_in,
        .out_cb = c64_cpu_port_out,
        .m6510_io_pullup = 0x17,
        .m6510_io_floating = 0xC8
    });

    /* initialize the CIAs */
    m6526_init(&c64.cia_1, c64_cia1_in, c64_cia1_out);
    m6526_init(&c64.cia_2, c64_cia2_in, c64_cia2_out);

    /* initialize the VIC-II display chip */
    m6569_init(&c64.vic, &(m6569_desc_t){
        .fetch_cb = c64_vic_fetch,
        .rgba8_buffer = rgba8_buffer,
        .rgba8_buffer_size = sizeof(rgba8_buffer),
        .vis_x = C64_DISP_X,
        .vis_y = C64_DISP_Y,
        .vis_w = C64_DISP_WIDTH,
        .vis_h = C64_DISP_HEIGHT
    });

    /* initialize the SID audio chip */
    m6581_init(&c64.sid, &(m6581_desc_t){
        .tick_hz = C64_FREQ,
        .sound_hz = 44100,
        .magnitude = 1.0
    });

    /* initial memory map

        the C64 has a weird RAM init pattern of 64 bytes 00 and 64 bytes FF
        alternating, probably with some randomness sprinkled in
        (see this thread: http://csdb.dk/forums/?roomid=11&topicid=116800&firstpost=2)
        this is important at least for the value of the 'ghost byte' at 0x3FFF,
        which is 0xFF
    */
    for (int i = 0; i < (1<<16); i++) {
        c64.ram[i] = (i & (1<<6)) ? 0xFF : 0x00;
    }

    /* setup the initial CPU memory map
       0000..9FFF and C000.CFFF is always RAM
    */
    mem_map_ram(&c64.mem_cpu, 0, 0x0000, 0xA000, c64.ram);
    mem_map_ram(&c64.mem_cpu, 0, 0xC000, 0x1000, c64.ram+0xC000);
    /* A000..BFFF, D000..DFFF and E000..FFFF are configurable */
    c64_update_memory_map();

    /* setup the separate VIC-II memory map (64 KByte RAM) overlayed with
       character ROMS at 0x1000.0x1FFF and 0x9000..0x9FFF
    */
    mem_map_ram(&c64.mem_vic, 1, 0x0000, 0x10000, c64.ram);
    mem_map_rom(&c64.mem_vic, 0, 0x1000, 0x1000, dump_c64_char);
    mem_map_rom(&c64.mem_vic, 0, 0x9000, 0x1000, dump_c64_char);

    /* put the CPU into start state */
    m6502_reset(&c64.cpu);

    /* setup the keyboard matrix
        http://sta.c64.org/cbm64kbdlay.html
        http://sta.c64.org/cbm64petkey.html
    */
    kbd_init(&c64.kbd, 1);
    const char* keymap =
        // no shift
        "        "
        "3WA4ZSE "
        "5RD6CFTX"
        "7YG8BHUV"
        "9IJ0MKON"
        "+PL-.:@,"
        "~*;  = /"  // ~ is actually the British Pound sign
        "1  2  Q "

        // shift
        "        "
        "#wa$zse "
        "%rd&cftx"
        "'yg(bhuv"
        ")ij0mkon"
        " pl >[ <"
        "$ ]    ?"
        "!  \"  q ";
    assert(strlen(keymap) == 128);
    /* shift is column 7, line 1 */
    kbd_register_modifier(&c64.kbd, 0, 7, 1);
    /* ctrl is column 2, line 7 */
    kbd_register_modifier(&c64.kbd, 1, 2, 7);
    for (int shift = 0; shift < 2; shift++) {
        for (int col = 0; col < 8; col++) {
            for (int line = 0; line < 8; line++) {
                int c = keymap[shift*64 + line*8 + col];
                if (c != 0x20) {
                    kbd_register_key(&c64.kbd, c, col, line, shift?(1<<0):0);
                }
            }
        }
    }

    /* special keys */
    kbd_register_key(&c64.kbd, 0x20, 4, 7, 0);    // space
    kbd_register_key(&c64.kbd, 0x08, 2, 0, 1);    // cursor left
    kbd_register_key(&c64.kbd, 0x09, 2, 0, 0);    // cursor right
    kbd_register_key(&c64.kbd, 0x0A, 7, 0, 0);    // cursor down
    kbd_register_key(&c64.kbd, 0x0B, 7, 0, 1);    // cursor up
    kbd_register_key(&c64.kbd, 0x01, 0, 0, 0);    // delete
    kbd_register_key(&c64.kbd, 0x0C, 3, 6, 1);    // clear
    kbd_register_key(&c64.kbd, 0x0D, 1, 0, 0);    // return
    kbd_register_key(&c64.kbd, 0x03, 7, 7, 0);    // stop
    kbd_register_key(&c64.kbd, 0xF1, 4, 0, 0);
    kbd_register_key(&c64.kbd, 0xF2, 4, 0, 1);
    kbd_register_key(&c64.kbd, 0xF3, 5, 0, 0);
    kbd_register_key(&c64.kbd, 0xF4, 5, 0, 1);
    kbd_register_key(&c64.kbd, 0xF5, 6, 0, 0);
    kbd_register_key(&c64.kbd, 0xF6, 6, 0, 1);
    kbd_register_key(&c64.kbd, 0xF7, 3, 0, 0);
    kbd_register_key(&c64.kbd, 0xF8, 3, 0, 1);
}

uint64_t c64_cpu_tick(uint64_t pins) {
    const uint16_t addr = M6502_GET_ADDR(pins);

    /* FIXME: tick the datasette, when the datasette output pulse
       toggles, the FLAG input pin on CIA-1 will go active for 1 tick
    */
    uint64_t cia1_pins = pins;
    /*
    if (c64_tape_tick()) {
        cia1_pins |= M6526_FLAG;
    }
    */

    /* tick the SID */
    if (m6581_tick(&c64.sid)) {
        /* FIXME: new sample ready, copy to audio buffer */
    }

    /* tick the CIAs:
        - CIA-1 gets the FLAG pin from the datasette
        - the CIA-1 IRQ pin is connected to the CPU IRQ pin
        - the CIA-2 IRQ pin is connected to the CPU NMI pin
    */
    if (m6526_tick(&c64.cia_1, cia1_pins & ~M6502_IRQ) & M6502_IRQ) {
        pins |= M6502_IRQ;
    }
    if (m6526_tick(&c64.cia_2, pins & ~M6502_IRQ) & M6502_IRQ) {
        pins |= M6502_NMI;
    }

    /* tick the VIC-II display chip:
        - the VIC-II IRQ pin is connected to the CPU IRQ pin and goes
        active when the VIC-II requests a rasterline interrupt
        - the VIC-II BA pin is connected to the CPU RDY pin, and stops
        the CPU on the first CPU read access after BA goes active
        - the VIC-II AEC pin is connected to the CPU AEC pin, currently
        this goes active during a badline, but is not checked
    */
    pins = m6569_tick(&c64.vic, pins);

    /* Special handling when the VIC-II asks the CPU to stop during a
        'badline' via the BA=>RDY pin. If the RDY pin is active, the
        CPU will 'loop' on the tick callback during the next READ access
        until the RDY pin goes inactive. The tick callback must make sure
        that only a single READ access happens during the entire RDY period.
        Currently this happens on the very last tick when RDY goes from
        active to inactive (I haven't found definitive documentation if
        this is the right behaviour, but it made the Boulderdash fast loader work).
    */
    if ((pins & (M6502_RDY|M6502_RW)) == (M6502_RDY|M6502_RW)) {
        return pins;
    }

    /* handle IO requests */
    if (M6510_CHECK_IO(pins)) {
        /* ...the integrated IO port in the M6510 CPU at addresses 0 and 1 */
        pins = m6510_iorq(&c64.cpu, pins);
    }
    else {
        /* ...the memory-mapped IO area from 0xD000 to 0xDFFF */
        if (c64.io_mapped && ((addr & 0xF000) == 0xD000)) {
            if (addr < 0xD400) {
                /* VIC-II (D000..D3FF) */
                uint64_t vic_pins = (pins & M6502_PIN_MASK)|M6569_CS;
                pins = m6569_iorq(&c64.vic, vic_pins) & M6502_PIN_MASK;
            }
            else if (addr < 0xD800) {
                /* SID (D400..D7FF) */
                uint64_t sid_pins = (pins & M6502_PIN_MASK)|M6581_CS;
                pins = m6581_iorq(&c64.sid, sid_pins) & M6502_PIN_MASK;
            }
            else if (addr < 0xDC00) {
                /* read or write the special color Static-RAM bank (D800..DBFF) */
                if (pins & M6502_RW) {
                    M6502_SET_DATA(pins, c64.color_ram[addr & 0x03FF]);
                }
                else {
                    c64.color_ram[addr & 0x03FF] = M6502_GET_DATA(pins);
                }
            }
            else if (addr < 0xDD00) {
                /* CIA-1 (DC00..DCFF) */
                uint64_t cia_pins = (pins & M6502_PIN_MASK)|M6526_CS;
                pins = m6526_iorq(&c64.cia_1, cia_pins) & M6502_PIN_MASK;
            }
            else if (addr < 0xDE00) {
                /* CIA-2 (DD00..DDFF) */
                uint64_t cia_pins = (pins & M6502_PIN_MASK)|M6526_CS;
                pins = m6526_iorq(&c64.cia_2, cia_pins) & M6502_PIN_MASK;
            }
            else {
                /* FIXME: expansion system (not implemented) */
            }
        }
        else {
            /* a regular memory access */
            if (pins & M6502_RW) {
                /* memory read */
                M6502_SET_DATA(pins, mem_rd(&c64.mem_cpu, addr));
            }
            else {
                /* memory write */
                mem_wr(&c64.mem_cpu, addr, M6502_GET_DATA(pins));
            }
        }
    }
    return pins;
}

uint8_t c64_cpu_port_in() {
    /*
        Input from the integrated M6510 CPU IO port

        bit 4: [in] datasette button status (1: no button pressed)
    */
    uint8_t val = 7;
    /* FIXME: datasette not implemented
    if (!tape_playing) {
        val |= (1<<4);
    }
    */
    return val;
}

void c64_cpu_port_out(uint8_t data) {
    /*
        Output to the integrated M6510 CPU IO port

        bits 0..2:  [out] memory configuration

        bit 3: [out] datasette output signal level
        bit 5: [out] datasette motor control (1: motor off)
    */
    /* FIXME: datasette not implemented
    if (data & (1<<5)) {
        tape_stop_motor();
    }
    else {
        tape_start_motor();
    }
    */
    /* only update memory configuration if the relevant bits have changed */
    bool need_mem_update = 0 != ((c64.cpu_port ^ data) & 7);
    c64.cpu_port = data;
    if (need_mem_update) {
        c64_update_memory_map();
    }
}

void c64_cia1_out(int port_id, uint8_t data) {
    /*
        Write CIA-1 ports:

        port A:
            write keyboard matrix lines
        port B:
            ---
    */
    if (port_id == M6526_PORT_A) {
        kbd_set_active_lines(&c64.kbd, ~data);
    }
}

uint8_t c64_cia1_in(int port_id) {
    /*
        Read CIA-1 ports:

        Port A:
            joystick 2 input
        Port B:
            combined keyboard matrix columns and joystick 1
    */
    if (port_id == M6526_PORT_A) {
        /* joystick 2 not implemented */
        return ~0;
    }
    else {
        /* read keyboard matrix columns (joystick 1 not implemented) */
        return ~kbd_scan_columns(&c64.kbd);
    }
}

void c64_cia2_out(int port_id, uint8_t data) {
    /*
        Write CIA-2 ports:

        Port A:
            bits 0..1: VIC-II bank select:
                00: bank 3 C000..FFFF
                01: bank 2 8000..BFFF
                10: bank 1 4000..7FFF
                11: bank 0 0000..3FFF
            bit 2: RS-232 TXD Outout (not implemented)
            bit 3..5: serial bus output (not implemented)
            bit 6..7: input (see cia2_in)
        Port B:
            RS232 / user functionality (not implemented)
    */
    if (port_id == M6526_PORT_A) {
        c64.vic_bank_select = ((~data)&3)<<14;
    }
}

uint8_t c64_cia2_in(int port_id) {
    /*
        Read CIA-2 ports:

        Port A:
            bits 0..5: output (see cia2_out)
            bits 6..7: serial bus input, not implemented
        Port B:
            RS232 / user functionality (not implemented)
    */
    return ~0;
}

uint16_t c64_vic_fetch(uint16_t addr) {
    /*
        Fetch data into the VIC-II.

        The VIC-II has a 14-bit address bus and 12-bit data bus, and
        has a different memory mapping then the CPU (that's why it
        goes through the mem_vic pagetable):
            - a full 16-bit address is formed by taking the address bits
              14 and 15 from the value written to CIA-1 port A
            - the lower 8 bits of the VIC-II data bus are connected
              to the shared system data bus, this is used to read
              character mask and pixel data
            - the upper 4 bits of the VIC-II data bus are hardwired to the
              static color RAM
    */
    addr |= c64.vic_bank_select;
    uint16_t data = (c64.color_ram[addr & 0x03FF]<<8) | mem_rd(&c64.mem_vic, addr);
    return data;
}

void c64_update_memory_map() {
    c64.io_mapped = false;
    uint8_t* read_ptr;
    const uint8_t charen = (1<<2);
    const uint8_t hiram = (1<<1);
    const uint8_t loram = (1<<0);
    /* shortcut if HIRAM and LORAM is 0, everything is RAM */
    if ((c64.cpu_port & (hiram|loram)) == 0) {
        mem_map_ram(&c64.mem_cpu, 0, 0xA000, 0x6000, c64.ram+0xA000);
    }
    else {
        /* A000..BFFF is either RAM-behind-BASIC-ROM or RAM */
        if ((c64.cpu_port & (hiram|loram)) == (hiram|loram)) {
            read_ptr = dump_c64_basic;
        }
        else {
            read_ptr = c64.ram + 0xA000;
        }
        mem_map_rw(&c64.mem_cpu, 0, 0xA000, 0x2000, read_ptr, c64.ram+0xA000);

        /* E000..FFFF is either RAM-behind-KERNAL-ROM or RAM */
        if (c64.cpu_port & hiram) {
            read_ptr = dump_c64_kernalv3;
        }
        else {
            read_ptr = c64.ram + 0xE000;
        }
        mem_map_rw(&c64.mem_cpu, 0, 0xE000, 0x2000, read_ptr, c64.ram+0xE000);

        /* D000..DFFF can be Char-ROM or I/O */
        if  (c64.cpu_port & charen) {
            c64.io_mapped = true;
        }
        else {
            mem_map_rw(&c64.mem_cpu, 0, 0xD000, 0x1000, dump_c64_char, c64.ram+0xD000);
        }
    }
}

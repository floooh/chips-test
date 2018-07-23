/*
    atom.c

    The Acorn Atom was a very simple 6502-based home computer
    (just a MOS 6502 CPU, Motorola MC6847 video
    display generator, and Intel i8255 I/O chip).

    Note: Ctrl+L (clear screen) is mapped to F1.

    NOT EMULATED:
        - the audio beeper
        - the optional VIA 6522
        - REPT key (and some other special keys)
*/
#include "sokol_app.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/mc6847.h"
#include "chips/i8255.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "roms/atom-roms.h"
#include "common/gfx.h"
#include "common/clock.h"
#include <ctype.h> /* isupper, islower, toupper, tolower */

/* Atom emulator state and callbacks */
#define ATOM_FREQ (1000000)
typedef struct {
    m6502_t cpu;
    mc6847_t vdg;
    i8255_t ppi;
    kbd_t kbd;
    mem_t mem;
    int counter_2_4khz;
    int period_2_4khz;
    bool state_2_4khz;
    uint8_t ram[1<<16];     /* only 40 KByte used */
} atom_t;
atom_t _atom;

void atom_init(atom_t* atom);
uint64_t atom_cpu_tick(uint64_t pins, void* user_data);
uint64_t atom_vdg_fetch(uint64_t pins, void* user_data);
uint8_t atom_ppi_in(int port_id, void* user_data);
uint64_t atom_ppi_out(int port_id, uint64_t pins, uint8_t data, void* user_data);

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
sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * MC6847_DISPLAY_WIDTH,
        .height = 2 * MC6847_DISPLAY_HEIGHT,
        .window_title = "Acorn Atom",
        .ios_keyboard_resizes_canvas = true
    };
}

/* one-time application init */
void app_init(void) {
    atom_t* atom = &_atom;
    gfx_init(MC6847_DISPLAY_WIDTH, MC6847_DISPLAY_HEIGHT, 1, 1);
    clock_init(ATOM_FREQ);
    atom_init(atom);
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame(void) {
    atom_t* atom = &_atom;
    clock_ticks_executed(m6502_exec(&atom->cpu, clock_ticks_to_run()));
    kbd_update(&atom->kbd);
    gfx_draw();
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    atom_t* atom = &_atom;
    int c = 0;
    switch (event->type) {
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
                kbd_key_down(&atom->kbd, c);
                kbd_key_up(&atom->kbd, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_UP:
        case SAPP_EVENTTYPE_KEY_DOWN:
            switch (event->key_code) {
                case SAPP_KEYCODE_ENTER:        c = 0x0D; break;
                case SAPP_KEYCODE_RIGHT:        c = 0x09; break;
                case SAPP_KEYCODE_LEFT:         c = 0x08; break;
                case SAPP_KEYCODE_DOWN:         c = 0x0A; break;
                case SAPP_KEYCODE_UP:           c = 0x0B; break;
                case SAPP_KEYCODE_INSERT:       c = 0x1A; break;
                case SAPP_KEYCODE_HOME:         c = 0x19; break;
                case SAPP_KEYCODE_BACKSPACE:    c = 0x01; break;
                case SAPP_KEYCODE_ESCAPE:       c = 0x1B; break;
                case SAPP_KEYCODE_F1:           c = 0x0C; break; /* mapped to Ctrl+L (clear screen) */
                default:                        c = 0;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    kbd_key_down(&atom->kbd, c);
                }
                else {
                    kbd_key_up(&atom->kbd, c);
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

/* Atom emulator initialization */
void atom_init(atom_t* atom) {
    /* setup memory map, first fill memory with random values */
    for (int i = 0; i < (int)sizeof(atom->ram);) {
        uint32_t r = xorshift32();
        atom->ram[i++]=r>>24; atom->ram[i++]=r>>16; atom->ram[i++]=r>>8; atom->ram[i++]=r;
    }
    mem_init(&atom->mem);
    /* 32 KByte RAM + 8 KByte vidmem */
    mem_map_ram(&atom->mem, 0, 0x0000, 0xA000, atom->ram);
    /* hole in 0xA000 to 0xAFFF for utility roms */
    /* 0xB000 to 0xBFFF is memory-mapped IO area (not mapped to host memory) */
    /* 0xC000 to 0xFFFF are operating system roms */
    mem_map_rom(&atom->mem, 0, 0xC000, 0x1000, dump_abasic);
    mem_map_rom(&atom->mem, 0, 0xD000, 0x1000, dump_afloat);
    mem_map_rom(&atom->mem, 0, 0xE000, 0x1000, dump_dosrom);
    mem_map_rom(&atom->mem, 0, 0xF000, 0x1000, dump_abasic+0x1000);

    /*  setup the keyboard matrix
        the Atom has a 10x8 keyboard matrix, where the
        entire line 6 is for the Ctrl key, and the entire
        line 7 is the Shift key
    */
    kbd_init(&atom->kbd, 1);
    /* shift key is entire line 7 */
    const int shift = (1<<0); kbd_register_modifier_line(&atom->kbd, 0, 7);
    /* ctrl key is entire line 6 */
    const int ctrl = (1<<1); kbd_register_modifier_line(&atom->kbd, 1, 6);
    /* alpha-numeric keys */
    const char* keymap = 
        /* no shift */
        "     ^]\\[ "/**/"3210      "/* */"-,;:987654"/**/"GFEDCBA@/."/**/"QPONMLKJIH"/**/" ZYXWVUTSR"
        /* shift */
        "          "/* */"#\"!       "/**/"=<+*)('&%$"/**/"gfedcba ?>"/**/"qponmlkjih"/**/" zyxwvutsr";
    for (int layer = 0; layer < 2; layer++) {
        for (int col = 0; col < 10; col++) {
            for (int line = 0; line < 6; line++) {
                int c = keymap[layer*60 + line*10 + col];
                if (c != 0x20) {
                    kbd_register_key(&atom->kbd, c, col, line, layer?shift:0);
                }
            }
        }
    }
    /* special keys */
    kbd_register_key(&atom->kbd, 0x20, 9, 0, 0);      /* space */
    kbd_register_key(&atom->kbd, 0x01, 4, 1, 0);      /* backspace */
    kbd_register_key(&atom->kbd, 0x08, 3, 0, shift);  /* left */
    kbd_register_key(&atom->kbd, 0x09, 3, 0, 0);      /* right */
    kbd_register_key(&atom->kbd, 0x0A, 2, 0, shift);  /* down */
    kbd_register_key(&atom->kbd, 0x0B, 2, 0, 0);      /* up */
    kbd_register_key(&atom->kbd, 0x0D, 6, 1, 0);      /* return/enter */
    kbd_register_key(&atom->kbd, 0x1B, 0, 5, 0);      /* escape */
    kbd_register_key(&atom->kbd, 0x0C, 5, 4, ctrl);   /* Ctrl+L, clear screen, mapped to F1 */

    /* initialize chips */
    m6502_init(&atom->cpu, &(m6502_desc_t){
        .tick_cb = atom_cpu_tick,
        .user_data = atom
    });
    mc6847_init(&atom->vdg, &(mc6847_desc_t){
        .tick_hz = ATOM_FREQ,
        .rgba8_buffer = rgba8_buffer,
        .rgba8_buffer_size = sizeof(rgba8_buffer),
        .fetch_cb = atom_vdg_fetch,
        .user_data = atom
    });
    i8255_init(&atom->ppi, &(i8255_desc_t){
        .in_cb = atom_ppi_in,
        .out_cb = atom_ppi_out,
        .user_data = atom
    });
    /* initialize 2.4 khz counter */
    atom->period_2_4khz = ATOM_FREQ / 2400;
    atom->counter_2_4khz = 0;
    atom->state_2_4khz = false;

    /* reset the CPU to go into 'start state' */
    m6502_reset(&atom->cpu);
}

/* CPU tick callback */
uint64_t atom_cpu_tick(uint64_t pins, void* user_data) {
    atom_t* atom = (atom_t*) user_data;

    /* tick the video chip */
    mc6847_tick(&atom->vdg);

    /* tick the 2.4khz counter */
    atom->counter_2_4khz++;
    if (atom->counter_2_4khz >= atom->period_2_4khz) {
        atom->state_2_4khz = !atom->state_2_4khz;
        atom->counter_2_4khz -= atom->period_2_4khz;
    }

    /* decode address for memory-mapped IO and memory read/write */
    const uint16_t addr = M6502_GET_ADDR(pins);
    if ((addr >= 0xB000) && (addr < 0xC000)) {
        /* memory-mapped IO area */
        if ((addr >= 0xB000) && (addr < 0xB400)) {
            /* i8255 PPI: http://www.acornatom.nl/sites/fpga/www.howell1964.freeserve.co.uk/acorn/atom/amb/amb_8255.htm */
            uint64_t ppi_pins = (pins & M6502_PIN_MASK) | I8255_CS;
            if (pins & M6502_RW) { ppi_pins |= I8255_RD; }  /* PPI read access */
            else { ppi_pins |= I8255_WR; }                  /* PPI write access */
            if (pins & M6502_A0) { ppi_pins |= I8255_A0; }  /* PPI has 4 addresses (port A,B,C or control word */
            if (pins & M6502_A1) { ppi_pins |= I8255_A1; }
            pins = i8255_iorq(&atom->ppi, ppi_pins) & M6502_PIN_MASK;
        }
        else {
            /* remaining IO space is for expansion devices */
            if (pins & M6502_RW) {
                M6502_SET_DATA(pins, 0x00);
            }
        }
    }
    else {
        /* memory access */
        if (pins & M6502_RW) {
            /* memory read */
            M6502_SET_DATA(pins, mem_rd(&atom->mem, addr));
        }
        else {
            /* memory access */
            mem_wr(&atom->mem, addr, M6502_GET_DATA(pins));
        }
    }
    return pins;
}

/* video memory fetch callback */
uint64_t atom_vdg_fetch(uint64_t pins, void* user_data) {
    atom_t* atom = (atom_t*) user_data;
    const uint16_t addr = MC6847_GET_ADDR(pins);
    uint8_t data = atom->ram[(addr + 0x8000) & 0xFFFF];
    MC6847_SET_DATA(pins, data);

    /*  the upper 2 databus bits are directly wired to MC6847 pins:
        bit 7 -> INV pin (in text mode, invert pixel pattern)
        bit 6 -> A/S and INT/EXT pin, A/S actives semigraphics mode
                 and INT/EXT selects the 2x3 semigraphics pattern
                 (so 4x4 semigraphics isn't possible)
    */
    if (data & (1<<7)) { pins |= MC6847_INV; }
    else               { pins &= ~MC6847_INV; }
    if (data & (1<<6)) { pins |= (MC6847_AS|MC6847_INTEXT); }
    else               { pins &= ~(MC6847_AS|MC6847_INTEXT); }
    return pins;
}

/* i8255 PPI output */
uint64_t atom_ppi_out(int port_id, uint64_t pins, uint8_t data, void* user_data) {
    atom_t* atom = (atom_t*) user_data;
    /*
        FROM Atom Theory and Praxis (and MAME)
        The  8255  Programmable  Peripheral  Interface  Adapter  contains  three
        8-bit ports, and all but one of these lines is used by the ATOM.
        Port A - #B000
               Output bits:      Function:
                    O -- 3     Keyboard column
                    4 -- 7     Graphics mode (4: A/G, 5..7: GM0..2)
        Port B - #B001
               Input bits:       Function:
                    O -- 5     Keyboard row
                      6        CTRL key (low when pressed)
                      7        SHIFT keys {low when pressed)
        Port C - #B002
               Output bits:      Function:
                    O          Tape output
                    1          Enable 2.4 kHz to cassette output
                    2          Loudspeaker
                    3          Not used (??? see below)
               Input bits:       Function:
                    4          2.4 kHz input
                    5          Cassette input
                    6          REPT key (low when pressed)
                    7          60 Hz sync signal (low during flyback)
        The port C output lines, bits O to 3, may be used for user
        applications when the cassette interface is not being used.
    */
    if (I8255_PORT_A == port_id) {
        /* PPI port A
            0..3:   keyboard matrix column to scan next
            4:      MC6847 A/G
            5:      MC6847 GM0
            6:      MC6847 GM1
            7:      MC6847 GM2
        */
        kbd_set_active_columns(&atom->kbd, 1<<(data & 0x0F));
        uint64_t vdg_pins = 0;
        uint64_t vdg_mask = MC6847_AG|MC6847_GM0|MC6847_GM1|MC6847_GM2;
        if (data & (1<<4)) { vdg_pins |= MC6847_AG; }
        if (data & (1<<5)) { vdg_pins |= MC6847_GM0; }
        if (data & (1<<6)) { vdg_pins |= MC6847_GM1; }
        if (data & (1<<7)) { vdg_pins |= MC6847_GM2; }
        mc6847_ctrl(&atom->vdg, vdg_pins, vdg_mask);
    }
    else if (I8255_PORT_C == port_id) {
        /* PPI port C output:
            0:  output: cass 0
            1:  output: cass 1
            2:  output: speaker
            3:  output: MC6847 CSS

            NOTE: only the MC6847 CSS pin is emulated here
        */
        uint64_t vdg_pins = 0;
        uint64_t vdg_mask = MC6847_CSS;
        if (data & (1<<3)) {
            vdg_pins |= MC6847_CSS;
        }
        mc6847_ctrl(&atom->vdg, vdg_pins, vdg_mask);
    }
    return pins;
}

/* i8255 PPI input callback */
uint8_t atom_ppi_in(int port_id, void* user_data) {
    atom_t* atom = (atom_t*) user_data;
    uint8_t data = 0;
    if (I8255_PORT_B == port_id) {
        /* keyboard row state */
        data = ~kbd_scan_lines(&atom->kbd);
    }
    else if (I8255_PORT_C == port_id) {
        /*  PPI port C input:
            4:  input: 2400 Hz
            5:  input: cassette
            6:  input: keyboard repeat
            7:  input: MC6847 FSYNC

            NOTE: only the 2400 Hz oscillator and FSYNC pins is emulated here
        */
        if (atom->state_2_4khz) {
            data |= (1<<4);
        }
        /* FIXME: always send REPEAT key as 'not pressed' */
        data |= (1<<6);
        /* vblank pin (cleared during vblank) */
        if (0 == (atom->vdg.pins & MC6847_FS)) {
            data |= (1<<7);
        }
    }
    return data;
}

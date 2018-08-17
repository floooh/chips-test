/*
    atom.c

    The Acorn Atom was a very simple 6502-based home computer
    (just a MOS 6502 CPU, Motorola MC6847 video
    display generator, and Intel i8255 I/O chip).

    Note: Ctrl+L (clear screen) is mapped to F1.

    NOT EMULATED:
        - REPT key (and some other special keys)
*/
#include "sokol_app.h"
#include "sokol_audio.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/mc6847.h"
#include "chips/i8255.h"
#include "chips/m6522.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/atom.h"
#include "common/gfx.h"
#include "common/fs.h"
#include "common/args.h"
#include "common/clock.h"
#include "roms/atom-roms.h"
#include <ctype.h> /* isupper, islower, toupper, tolower */

atom_t atom;

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
        .width = 2 * ATOM_DISPLAY_WIDTH,
        .height = 2 * ATOM_DISPLAY_HEIGHT,
        .window_title = "Acorn Atom",
        .ios_keyboard_resizes_canvas = true
    };
}

/* one-time application init */
void app_init(void) {
    gfx_init(ATOM_DISPLAY_WIDTH, ATOM_DISPLAY_HEIGHT, 1, 1);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    atom_joystick_t joy_type = ATOM_JOYSTICK_NONE;
    if (args_has("joystick")) {
        if (args_string_compare("joystick", "mmc") ||
            args_string_compare("joystick", "yes"))
        {
            joy_type = ATOM_JOYSTICK_MMC;
        }
    }
    atom_init(&atom, &(atom_desc_t){
        .joystick_type = joy_type,
        .audio_cb = saudio_push,
        .audio_sample_rate = saudio_sample_rate(),
        .pixel_buffer = rgba8_buffer,
        .pixel_buffer_size = sizeof(rgba8_buffer),
        .rom_abasic = dump_abasic,
        .rom_abasic_size = sizeof(dump_abasic),
        .rom_afloat = dump_afloat,
        .rom_afloat_size = sizeof(dump_afloat),
        .rom_dosrom = dump_dosrom,
        .rom_dosrom_size = sizeof(dump_dosrom)
    });
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    atom_exec(&atom, clock_frame_time());
    gfx_draw();
    /*
    FIXME!
    if (fs_ptr() && clock_frame_count() > 120) {
        atom_quickload(&atom, fs_ptr(), fs_size());
        fs_free();
    }
    */
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
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
                kbd_key_down(&atom.kbd, c);
                kbd_key_up(&atom.kbd, c);
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
                    kbd_key_down(&atom.kbd, c);
                }
                else {
                    kbd_key_up(&atom.kbd, c);
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
    atom_discard(&atom);
    saudio_shutdown();
    gfx_shutdown();
}

/* CPU tick callback */
uint64_t _atom_tick(uint64_t pins, void* user_data) {
    atom_t* sys = (atom_t*) user_data;

    /* tick the video chip */
    mc6847_tick(&sys->vdg);

    /* tick the 6522 VIA */
    m6522_tick(&sys->via);

    /* tick the 2.4khz counter */
    sys->counter_2_4khz++;
    if (sys->counter_2_4khz >= sys->period_2_4khz) {
        sys->state_2_4khz = !sys->state_2_4khz;
        sys->counter_2_4khz -= sys->period_2_4khz;
    }

    /* update beeper */
    if (beeper_tick(&sys->beeper)) {
        /* new audio sample ready */
        sys->sample_buffer[sys->sample_pos++] = sys->beeper.sample;
        if (sys->sample_pos == sys->num_samples) {
            if (sys->audio_cb) {
                sys->audio_cb(sys->sample_buffer, sys->num_samples);
            }
            sys->sample_pos = 0;
        }
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
            pins = i8255_iorq(&sys->ppi, ppi_pins) & M6502_PIN_MASK;
        }       
        else if ((addr >= 0xB400) && (addr < 0xB800)) {
            /* extensions (only rudimentary)
                FIXME: implement a proper AtoMMC emulation, for now just
                a quick'n'dirty hack for joystick input
            */
            if (pins & M6502_RW) {
                /* read from MMC extension */
                if (addr == 0xB400) {
                    /* reading from 0xB400 returns a status/error code, the important
                        ones are STATUS_OK=0x3F, and STATUS_BUSY=0x80, STATUS_COMPLETE
                        together with an error code is used to communicate errors
                    */
                    M6502_SET_DATA(pins, 0x3F);
                }
                else if ((addr == 0xB401) && (sys->mmc_cmd == 0xA2)) {
                    /* read MMC joystick */
                    M6502_SET_DATA(pins, ~sys->mmc_joymask);
                }
            }
            else {
                /* write to MMC extension */
                if (addr == 0xB400) {
                    sys->mmc_cmd = M6502_GET_DATA(pins);
                }
            }
        } 
        else if ((addr >= 0xB800) && (addr < 0xBC00)) {
            /* 6522 VIA: http://www.acornatom.nl/sites/fpga/www.howell1964.freeserve.co.uk/acorn/atom/amb/amb_6522.htm */
            uint64_t via_pins = (pins & M6502_PIN_MASK)|M6522_CS1;
            /* NOTE: M6522_RW pin is identical with M6502_RW) */
            pins = m6522_iorq(&sys->via, via_pins) & M6502_PIN_MASK;
        }
        else {
            /* remaining IO space is for expansion devices */
            if (pins & M6502_RW) {
                M6502_SET_DATA(pins, 0x00);
            }
        }
    }
    else {
        /* regular memory access */
        if (pins & M6502_RW) {
            /* memory read */
            M6502_SET_DATA(pins, mem_rd(&sys->mem, addr));
        }
        else {
            /* memory access */
            mem_wr(&sys->mem, addr, M6502_GET_DATA(pins));
        }
    }
    return pins;
}

/* video memory fetch callback */
uint64_t _atom_vdg_fetch(uint64_t pins, void* user_data) {
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
uint64_t _atom_ppi_out(int port_id, uint64_t pins, uint8_t data, void* user_data) {
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
uint8_t _atom_ppi_in(int port_id, void* user_data) {
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

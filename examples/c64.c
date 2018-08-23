/*
    c64.c
*/
#include "common/common.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/c64.h"
#include "roms/c64-roms.h"

c64_t c64;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    args_init(argc, argv);
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * C64_DISPLAY_WIDTH,
        .height = 2 * C64_DISPLAY_HEIGHT,
        .window_title = "C64",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* one-time application init */
void app_init(void) {
    gfx_init(C64_DISPLAY_WIDTH, C64_DISPLAY_HEIGHT, 1, 1);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
    c64_joystick_t joy_type = C64_JOYSTICK_NONE;
    if (args_has("joystick")) {
        joy_type = C64_JOYSTICK_DIGITAL;
    }
    c64_init(&c64, &(c64_desc_t){
        .joystick_type = joy_type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .rom_char = dump_c64_char,
        .rom_char_size = sizeof(dump_c64_char),
        .rom_basic = dump_c64_basic,
        .rom_basic_size = sizeof(dump_c64_basic),
        .rom_kernal = dump_c64_kernalv3,
        .rom_kernal_size = sizeof(dump_c64_kernalv3)
    });
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame(void) {
    c64_exec(&c64, clock_frame_time());
    gfx_draw();
    /* FIXME: file loading */
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
        case SAPP_EVENTTYPE_TOUCHES_BEGAN:
            sapp_show_keyboard(true);
            break;
        default:
            break;
    }
}

/* application cleanup callback */
void app_cleanup(void) {
    c64_discard(&c64);
    saudio_shutdown();
    gfx_shutdown();
}

uint64_t _c64_tick(uint64_t pins, void* user_data) {
    c64_t* sys = (c64_t*) user_data;
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
    if (m6581_tick(&sys->sid)) {
        /* FIXME: new sample ready, copy to audio buffer */
    }

    /* tick the CIAs:
        - CIA-1 gets the FLAG pin from the datasette
        - the CIA-1 IRQ pin is connected to the CPU IRQ pin
        - the CIA-2 IRQ pin is connected to the CPU NMI pin
    */
    if (m6526_tick(&sys->cia_1, cia1_pins & ~M6502_IRQ) & M6502_IRQ) {
        pins |= M6502_IRQ;
    }
    if (m6526_tick(&sys->cia_2, pins & ~M6502_IRQ) & M6502_IRQ) {
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
    pins = m6569_tick(&sys->vic, pins);

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
        pins = m6510_iorq(&sys->cpu, pins);
    }
    else {
        /* ...the memory-mapped IO area from 0xD000 to 0xDFFF */
        if (sys->io_mapped && ((addr & 0xF000) == 0xD000)) {
            if (addr < 0xD400) {
                /* VIC-II (D000..D3FF) */
                uint64_t vic_pins = (pins & M6502_PIN_MASK)|M6569_CS;
                pins = m6569_iorq(&sys->vic, vic_pins) & M6502_PIN_MASK;
            }
            else if (addr < 0xD800) {
                /* SID (D400..D7FF) */
                uint64_t sid_pins = (pins & M6502_PIN_MASK)|M6581_CS;
                pins = m6581_iorq(&sys->sid, sid_pins) & M6502_PIN_MASK;
            }
            else if (addr < 0xDC00) {
                /* read or write the special color Static-RAM bank (D800..DBFF) */
                if (pins & M6502_RW) {
                    M6502_SET_DATA(pins, sys->color_ram[addr & 0x03FF]);
                }
                else {
                    sys->color_ram[addr & 0x03FF] = M6502_GET_DATA(pins);
                }
            }
            else if (addr < 0xDD00) {
                /* CIA-1 (DC00..DCFF) */
                uint64_t cia_pins = (pins & M6502_PIN_MASK)|M6526_CS;
                pins = m6526_iorq(&sys->cia_1, cia_pins) & M6502_PIN_MASK;
            }
            else if (addr < 0xDE00) {
                /* CIA-2 (DD00..DDFF) */
                uint64_t cia_pins = (pins & M6502_PIN_MASK)|M6526_CS;
                pins = m6526_iorq(&sys->cia_2, cia_pins) & M6502_PIN_MASK;
            }
            else {
                /* FIXME: expansion system (not implemented) */
            }
        }
        else {
            /* a regular memory access */
            if (pins & M6502_RW) {
                /* memory read */
                M6502_SET_DATA(pins, mem_rd(&sys->mem_cpu, addr));
            }
            else {
                /* memory write */
                mem_wr(&sys->mem_cpu, addr, M6502_GET_DATA(pins));
            }
        }
    }
    return pins;
}

uint8_t _c64_cpu_port_in(void* user_data) {
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

void _c64_cpu_port_out(uint8_t data, void* user_data) {
    c64_t* sys = (c64_t*) user_data;
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
    bool need_mem_update = 0 != ((sys->cpu_port ^ data) & 7);
    sys->cpu_port = data;
    if (need_mem_update) {
        _c64_update_memory_map(sys);
    }
}

void _c64_cia1_out(int port_id, uint8_t data, void* user_data) {
    c64_t* sys = (c64_t*) user_data;
    /*
        Write CIA-1 ports:

        port A:
            write keyboard matrix lines
        port B:
            ---
    */
    if (port_id == M6526_PORT_A) {
        kbd_set_active_lines(&sys->kbd, ~data);
    }
}

uint8_t _c64_cia1_in(int port_id, void* user_data) {
    c64_t* sys = (c64_t*) user_data;
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
        return ~kbd_scan_columns(&sys->kbd);
    }
}

void _c64_cia2_out(int port_id, uint8_t data, void* user_data) {
    c64_t* sys = (c64_t*) user_data;
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
        sys->vic_bank_select = ((~data)&3)<<14;
    }
}

uint8_t _c64_cia2_in(int port_id, void* user_data) {
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

uint16_t _c64_vic_fetch(uint16_t addr, void* user_data) {
    c64_t* sys = (c64_t*) user_data;
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
    addr |= sys->vic_bank_select;
    uint16_t data = (sys->color_ram[addr & 0x03FF]<<8) | mem_rd(&sys->mem_vic, addr);
    return data;
}

/*
    cpc.c

    Amstrad CPC 464/6128 and KC Comptact. No disc emulation.
*/
#include "sokol_app.h"
#include "sokol_audio.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/i8255.h"
#include "chips/mc6845.h"
#include "chips/crt.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/cpc.h"
#include "common/common.h"
#include "roms/cpc-roms.h"

cpc_t cpc;

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
    if (args_has("tape")) {
        /* FIXME: TAPE LOADING */
    }
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = CPC_DISPLAY_WIDTH,
        .height = 2 * CPC_DISPLAY_HEIGHT,
        .window_title = "CPC 6128",
        .ios_keyboard_resizes_canvas = true
    };
}

/* one-time application init */
void app_init(void) {
    gfx_init(CPC_DISPLAY_WIDTH, CPC_DISPLAY_HEIGHT, 1, 2);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    cpc_type_t type = CPC_TYPE_6128;
    if (args_has("type")) {
        if (args_string_compare("type", "cpc464")) {
            type = CPC_TYPE_464;
        }
        else if (args_string_compare("type", "kccompact")) {
            type = CPC_TYPE_KCCOMPACT;
        }
    }
    cpc_joystick_t joy_type = CPC_JOYSTICK_NONE;
    if (args_has("joystick")) {
        joy_type = CPC_JOYSTICK_DIGITAL;
    }
    cpc_init(&cpc, &(cpc_desc_t){
        .type = type,
        .joystick_type = joy_type,
        .pixel_buffer = rgba8_buffer,
        .pixel_buffer_size = sizeof(rgba8_buffer),
        .audio_cb = saudio_push,
        .audio_sample_rate = saudio_sample_rate(),
        .rom_464_os = dump_cpc464_os,
        .rom_464_os_size = sizeof(dump_cpc464_os),
        .rom_464_basic = dump_cpc464_basic,
        .rom_464_basic_size = sizeof(dump_cpc464_basic),
        .rom_6128_os = dump_cpc6128_os,
        .rom_6128_os_size = sizeof(dump_cpc6128_os),
        .rom_6128_basic = dump_cpc6128_basic,
        .rom_6128_basic_size = sizeof(dump_cpc6128_basic),
        .rom_6128_amsdos = dump_cpc6128_amsdos,
        .rom_6128_amsdos_size = sizeof(dump_cpc6128_amsdos),
        .rom_kcc_os = dump_kcc_os,
        .rom_kcc_os_size = sizeof(dump_kcc_os),
        .rom_kcc_basic = dump_kcc_bas,
        .rom_kcc_basic_size = sizeof(dump_kcc_bas)
    });
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame(void) {
    cpc_exec(&cpc, clock_frame_time());
    gfx_draw();
    /* FIXME
    if (fs_ptr() && clock_frame_count() > 20) {
        cpc_load_sna(cpc, fs_ptr(), fs_size());
        fs_free();
    }
    */
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    cpc_t* sys = &cpc;
    const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if (c < KBD_MAX_KEYS) {
                kbd_key_down(&sys->kbd, c);
                kbd_key_up(&sys->kbd, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_SPACE:        c = sys->joystick_type ? 0 : 0x20; break;
                case SAPP_KEYCODE_LEFT:         c = sys->joystick_type ? 0 : 0x08; break;
                case SAPP_KEYCODE_RIGHT:        c = sys->joystick_type ? 0 : 0x09; break;
                case SAPP_KEYCODE_DOWN:         c = sys->joystick_type ? 0 : 0x0A; break;
                case SAPP_KEYCODE_UP:           c = sys->joystick_type ? 0 : 0x0B; break;
                case SAPP_KEYCODE_ENTER:        c = 0x0D; break;
                case SAPP_KEYCODE_LEFT_SHIFT:   c = 0x02; break;
                case SAPP_KEYCODE_BACKSPACE:    c = shift ? 0x0C : 0x01; break; // 0x0C: clear screen
                case SAPP_KEYCODE_ESCAPE:       c = shift ? 0x13 : 0x03; break; // 0x13: break
                case SAPP_KEYCODE_F1:           c = 0xF1; break;
                case SAPP_KEYCODE_F2:           c = 0xF2; break;
                case SAPP_KEYCODE_F3:           c = 0xF3; break;
                case SAPP_KEYCODE_F4:           c = 0xF4; break;
                case SAPP_KEYCODE_F5:           c = 0xF5; break;
                case SAPP_KEYCODE_F6:           c = 0xF6; break;
                case SAPP_KEYCODE_F7:           c = 0xF7; break;
                case SAPP_KEYCODE_F8:           c = 0xF8; break;
                case SAPP_KEYCODE_F9:           c = 0xF9; break;
                case SAPP_KEYCODE_F10:          c = 0xFA; break;
                case SAPP_KEYCODE_F11:          c = 0xFB; break;
                case SAPP_KEYCODE_F12:          c = 0xFC; break;
                default:                        c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    kbd_key_down(&sys->kbd, c);
                }
                else {
                    kbd_key_up(&sys->kbd, c);
                }
            }
            else if (sys->joystick_type) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    switch (event->key_code) {
                        case SAPP_KEYCODE_SPACE:        sys->joy_mask |= 1<<4; break;
                        case SAPP_KEYCODE_LEFT:         sys->joy_mask |= 1<<2; break;
                        case SAPP_KEYCODE_RIGHT:        sys->joy_mask |= 1<<3; break;
                        case SAPP_KEYCODE_DOWN:         sys->joy_mask |= 1<<1; break;
                        case SAPP_KEYCODE_UP:           sys->joy_mask |= 1<<0; break;
                        default: break;
                    }
                }
                else {
                    switch (event->key_code) {
                        case SAPP_KEYCODE_SPACE:        sys->joy_mask &= ~(1<<4); break;
                        case SAPP_KEYCODE_LEFT:         sys->joy_mask &= ~(1<<2); break;
                        case SAPP_KEYCODE_RIGHT:        sys->joy_mask &= ~(1<<3); break;
                        case SAPP_KEYCODE_DOWN:         sys->joy_mask &= ~(1<<1); break;
                        case SAPP_KEYCODE_UP:           sys->joy_mask &= ~(1<<0); break;
                        default: break;
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
void app_cleanup(void) {
    cpc_discard(&cpc);
    saudio_shutdown();
    gfx_shutdown();
}

uint64_t _cpc_tick(int num_ticks, uint64_t pins, void* user_data) {
    cpc_t* sys = (cpc_t*) user_data;

    /* interrupt acknowledge? */
    if ((pins & (Z80_M1|Z80_IORQ)) == (Z80_M1|Z80_IORQ)) {
        _cpc_ga_int_ack(sys);
    }

    /* memory and IO requests */
    if (pins & Z80_MREQ) {
        /* CPU MEMORY REQUEST */
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem_rd(&sys->mem, addr));
        }
        else if (pins & Z80_WR) {
            mem_wr(&sys->mem, addr, Z80_GET_DATA(pins));
        }
    }
    else if ((pins & Z80_IORQ) && (pins & (Z80_RD|Z80_WR))) {
        /* CPU IO REQUEST */
        pins = _cpc_cpu_iorq(sys, pins);
    }
    
    /*
        tick the gate array and audio chip at 1 MHz, and decide how many wait
        states must be injected, the CPC signals the wait line in 3 out of 4
        cycles:
    
         0: wait inactive
         1: wait active
         2: wait active
         3: wait active
    
        the CPU samples the wait line only on specific clock ticks
        during memory or IO operations, wait states are only injected
        if the 'wait active' happens on the same clock tick as the
        CPU would sample the wait line
    */
    int wait_scan_tick = -1;
    if (pins & Z80_MREQ) {
        /* a memory request or opcode fetch, wait is sampled on second clock tick */
        wait_scan_tick = 1;
    }
    else if (pins & Z80_IORQ) {
        if (pins & Z80_M1) {
            /* an interrupt acknowledge cycle, wait is sampled on fourth clock tick */
            wait_scan_tick = 3;
        }
        else {
            /* an IO cycle, wait is sampled on third clock tick */
            wait_scan_tick = 2;
        }
    }
    bool wait = false;
    uint32_t wait_cycles = 0;
    for (int i = 0; i<num_ticks; i++) {
        do {
            /* CPC gate array sets the wait pin for 3 out of 4 clock ticks */
            bool wait_pin = (sys->tick_count++ & 3) != 0;
            wait = (wait_pin && (wait || (i == wait_scan_tick)));
            if (wait) {
                wait_cycles++;
            }
            /* on every 4th clock cycle, tick the system */
            if (!wait_pin) {
                if (ay38910_tick(&sys->psg)) {
                    /* FIXME
                    sound_push(sys->psg.sample);
                    */
                }
                pins = _cpc_ga_tick(sys, pins);
            }
        }
        while (wait);
    }
    Z80_SET_WAIT(pins, wait_cycles);
    return pins;
}

uint64_t _cpc_cpu_iorq(cpc_t* sys, uint64_t pins) {
    /*
        CPU IO REQUEST

        For address decoding, see the main board schematics!
        also: http://cpcwiki.eu/index.php/Default_I/O_Port_Summary

        Z80 to i8255 PPI pin connections:
            ~A11 -> CS (CS is active-low)
                A8 -> A0
                A9 -> A1
                RD -> RD
                WR -> WR
            D0..D7 -> D0..D7
    */
    if ((pins & Z80_A11) == 0) {
        /* i8255 in/out */
        uint64_t ppi_pins = (pins & Z80_PIN_MASK)|I8255_CS;
        if (pins & Z80_A9) { ppi_pins |= I8255_A1; }
        if (pins & Z80_A8) { ppi_pins |= I8255_A0; }
        if (pins & Z80_RD) { ppi_pins |= I8255_RD; }
        if (pins & Z80_WR) { ppi_pins |= I8255_WR; }
        pins = i8255_iorq(&sys->ppi, ppi_pins) & Z80_PIN_MASK;
    }
    /*
        Z80 to MC6845 pin connections:

            ~A14 -> CS (CS is active low)
            A9  -> RW (high: read, low: write)
            A8  -> RS
        D0..D7  -> D0..D7
    */
    if ((pins & Z80_A14) == 0) {
        /* 6845 in/out */
        uint64_t vdg_pins = (pins & Z80_PIN_MASK)|MC6845_CS;
        if (pins & Z80_A9) { vdg_pins |= MC6845_RW; }
        if (pins & Z80_A8) { vdg_pins |= MC6845_RS; }
        pins = mc6845_iorq(&sys->vdg, vdg_pins) & Z80_PIN_MASK;
    }
    /*
        Gate Array Function (only writing to the gate array
        is possible, but the gate array doesn't check the
        CPU R/W pins, so each access is a write).

        This is used by the Arnold Acid test "OnlyInc", which
        access the PPI and gate array in the same IO operation
        to move data directly from the PPI into the gate array.
    */
    if ((pins & (Z80_A15|Z80_A14)) == Z80_A14) {
        /* D6 and D7 select the gate array operation */
        const uint8_t data = Z80_GET_DATA(pins);
        switch (data & ((1<<7)|(1<<6))) {
            case 0:
                /* select pen:
                    bit 4 set means 'select border', otherwise
                    bits 0..3 contain the pen number
                */
                sys->ga.pen = data & 0x1F;
                break;
            case (1<<6):
                /* select color for border or selected pen: */
                if (sys->ga.pen & (1<<4)) {
                    /* border color */
                    sys->ga.border_color = sys->ga.colors[data & 0x1F];
                }
                else {
                    sys->ga.palette[sys->ga.pen & 0x0F] = sys->ga.colors[data & 0x1F];
                }
                break;
            case (1<<7):
                /* select screen mode, ROM config and interrupt control
                    - bits 0 and 1 select the screen mode
                        00: Mode 0 (160x200 @ 16 colors)
                        01: Mode 1 (320x200 @ 4 colors)
                        02: Mode 2 (640x200 @ 2 colors)
                        11: Mode 3 (160x200 @ 2 colors, undocumented)
                  
                    - bit 2: disable/enable lower ROM
                    - bit 3: disable/enable upper ROM
                  
                    - bit 4: interrupt generation control
                */
                sys->ga.config = data;
                sys->ga.next_video_mode = data & 3;
                if (data & (1<<4)) {
                    sys->ga.hsync_irq_counter = 0;
                    sys->ga.intr = false;
                }
                _cpc_update_memory_mapping(sys);
                break;
            case (1<<7)|(1<<6):
                /* RAM memory management (only CPC6128) */
                if (CPC_TYPE_6128 == sys->type) {
                    sys->ga.ram_config = data;
                    _cpc_update_memory_mapping(sys);
                }
                break;
        }
    }
    /*
        Upper ROM Bank number

        This is used to select a ROM in the
        0xC000..0xFFFF region, without expansions,
        this is just the BASIC and AMSDOS ROM.
    */
    if ((pins & Z80_A13) == 0) {
        sys->upper_rom_select = Z80_GET_DATA(pins);
        _cpc_update_memory_mapping(sys);
    }
    /*
        Floppy Disk Interface
    */
    if ((pins & (Z80_A10|Z80_A8|Z80_A7)) == 0) {
        /* FIXME: floppy disk motor control */
    }
    else if ((pins & (Z80_A10|Z80_A8|Z80_A7)) == Z80_A8) {
        /* floppy controller status/data register */
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, 0xFF);
        }
    }
    return pins;
}

uint64_t _cpc_ppi_out(int port_id, uint64_t pins, uint8_t data, void* user_data) {
    cpc_t* sys = (cpc_t*) user_data;
    /*
        i8255 PPI to AY-3-8912 PSG pin connections:
            PA0..PA7    -> D0..D7
                 PC7    -> BDIR
                 PC6    -> BC1
    */
    if ((I8255_PORT_A == port_id) || (I8255_PORT_C == port_id)) {
        const uint8_t ay_ctrl = sys->ppi.output[I8255_PORT_C] & ((1<<7)|(1<<6));
        if (ay_ctrl) {
            uint64_t ay_pins = 0;
            if (ay_ctrl & (1<<7)) { ay_pins |= AY38910_BDIR; }
            if (ay_ctrl & (1<<6)) { ay_pins |= AY38910_BC1; }
            const uint8_t ay_data = sys->ppi.output[I8255_PORT_A];
            AY38910_SET_DATA(ay_pins, ay_data);
            ay38910_iorq(&sys->psg, ay_pins);
        }
    }
    if (I8255_PORT_C == port_id) {
        // bits 0..3: select keyboard matrix line
        kbd_set_active_columns(&sys->kbd, 1<<(data & 0x0F));

        /* FIXME: cassette write data */
        /* FIXME: cassette deck motor control */
        if (data & (1<<4)) {
            // tape_start_motor();
        }
        else {
            // tape_stop_motor();
        }
    }
    return pins;
}

uint8_t _cpc_ppi_in(int port_id, void* user_data) {
    cpc_t* sys = (cpc_t*) user_data;
    if (I8255_PORT_A == port_id) {
        /* AY-3-8912 PSG function (indirectly this may also trigger
            a read of the keyboard matrix via the AY's IO port
        */
        uint64_t ay_pins = 0;
        uint8_t ay_ctrl = sys->ppi.output[I8255_PORT_C];
        if (ay_ctrl & (1<<7)) ay_pins |= AY38910_BDIR;
        if (ay_ctrl & (1<<6)) ay_pins |= AY38910_BC1;
        uint8_t ay_data = sys->ppi.output[I8255_PORT_A];
        AY38910_SET_DATA(ay_pins, ay_data);
        ay_pins = ay38910_iorq(&sys->psg, ay_pins);
        return AY38910_GET_DATA(ay_pins);
    }
    else if (I8255_PORT_B == port_id) {
        /*
            Bit 7: cassette data input
            Bit 6: printer port ready (1=not ready, 0=ready)
            Bit 5: expansion port /EXP pin
            Bit 4: screen refresh rate (1=50Hz, 0=60Hz)
            Bit 3..1: distributor id (shown in start screen)
                0: Isp
                1: Triumph
                2: Saisho
                3: Solavox
                4: Awa
                5: Schneider
                6: Orion
                7: Amstrad
            Bit 0: vsync
        */
        uint8_t val = (1<<4) | (7<<1);    // 50Hz refresh rate, Amstrad
        /* PPI Port B Bit 0 is directly wired to the 6845 VSYNC pin (see schematics) */
        if (sys->vdg.vs) {
            val |= (1<<0);
        }
        return val;
    }
    else {
        /* shouldn't happen */
        return 0xFF;
    }
}

void _cpc_psg_out(int port_id, uint8_t data, void* user_data) {
    /* this shouldn't be called */
}

uint8_t _cpc_psg_in(int port_id, void* user_data) {
    cpc_t* sys = (cpc_t*) user_data;
    /* read the keyboard matrix and joystick port */
    if (port_id == AY38910_PORT_A) {
        uint8_t data = (uint8_t) kbd_scan_lines(&sys->kbd);
        if (sys->kbd.active_columns & (1<<9)) {
            /* FIXME: joystick input not implemented

                joystick input is implemented like this:
                - the keyboard column 9 is routed to the joystick
                  ports "COM1" pin, this means the joystick is only
                  "powered" when the keyboard line 9 is active
                - the joysticks direction and fire pins are
                  connected to the keyboard matrix lines as
                  input to PSG port A
                - thus, only when the keyboard matrix column 9 is sampled,
                  joystick input will be provided on the keyboard
                  matrix lines
            */
            data |= sys->joy_mask;
        }
        return ~data;
    }
    else {
        /* this shouldn't happen since the AY-3-8912 only has one IO port */
        return 0xFF;
    }
}

void _cpc_ga_int_ack(cpc_t* sys) {
    /* on interrupt acknowledge from the CPU, clear the top bit from the
        hsync counter, so the next interrupt can't occur closer then 
        32 HSYNC, and clear the gate array interrupt pin state
    */
    sys->ga.hsync_irq_counter &= 0x1F;
    sys->ga.intr = false;
}

static bool falling_edge(uint64_t new_pins, uint64_t old_pins, uint64_t mask) {
    return 0 != (mask & (~new_pins & (new_pins ^ old_pins)));
}

static bool rising_edge(uint64_t new_pins, uint64_t old_pins, uint64_t mask) {
    return 0 != (mask & (new_pins & (new_pins ^ old_pins)));
}

uint64_t _cpc_ga_tick(cpc_t* sys, uint64_t cpu_pins) {
    /*
        http://cpctech.cpc-live.com/docs/ints.html
        http://www.cpcwiki.eu/forum/programming/frame-flyback-and-interrupts/msg25106/#msg25106
        https://web.archive.org/web/20170612081209/http://www.grimware.org/doku.php/documentations/devices/gatearray
    */
    uint64_t crtc_pins = mc6845_tick(&sys->vdg);

    /*
        INTERRUPT GENERATION:

        From: https://web.archive.org/web/20170612081209/http://www.grimware.org/doku.php/documentations/devices/gatearray

        - On every falling edge of the HSYNC signal (from the 6845),
          the gate array will increment the counter by one. When the
          counter reaches 52, the gate array will raise the INT signal
          and reset the counter.
        - When the CPU acknowledges the interrupt, the gate array will
          reset bit5 of the counter, so the next interrupt can't occur
          closer than 32 HSync.
        - When a VSync occurs, the gate array will wait for 2 HSync and:
            - if the counter>=32 (bit5=1) then no interrupt request is issued
              and counter is reset to 0
            - if the counter<32 (bit5=0) then an interrupt request is issued
              and counter is reset to 0
        - This 2 HSync delay after a VSync is used to let the main program,
          executed by the CPU, enough time to sense the VSync...

        From: http://www.cpcwiki.eu/index.php?title=CRTC
        
        - The HSYNC is modified before being sent to the monitor. It happens
          2us after the HSYNC from the CRTC and lasts 4us when HSYNC length
          is greater or equal to 6.
        - The VSYNC is also modified before being sent to the monitor, it happens
          two lines after the VSYNC from the CRTC and stay 2 lines (same cut
          rule if VSYNC is lower than 4).

        NOTES:
            - the interrupt acknowledge is handled once per machine 
              cycle in cpu_tick()
            - the video mode will take effect *after the next HSYNC*
    */
    if (rising_edge(crtc_pins, sys->ga.crtc_pins, MC6845_VS)) {
        sys->ga.hsync_after_vsync_counter = 2;
    }
    if (falling_edge(crtc_pins, sys->ga.crtc_pins, MC6845_HS)) {
        sys->ga.video_mode = sys->ga.next_video_mode;
        sys->ga.hsync_irq_counter = (sys->ga.hsync_irq_counter + 1) & 0x3F;

        /* 2 HSync delay? */
        if (sys->ga.hsync_after_vsync_counter > 0) {
            sys->ga.hsync_after_vsync_counter--;
            if (sys->ga.hsync_after_vsync_counter == 0) {
                if (sys->ga.hsync_irq_counter >= 32) {
                    sys->ga.intr = true;
                }
                sys->ga.hsync_irq_counter = 0;
            }
        }
        /* normal behaviour, request interrupt each 52 scanlines */
        if (sys->ga.hsync_irq_counter == 52) {
            sys->ga.hsync_irq_counter = 0;
            sys->ga.intr = true;
        }
    }

    /* generate HSYNC signal to monitor:
        - starts 2 ticks after HSYNC rising edge from CRTC
        - stays active for 4 ticks or less if CRTC HSYNC goes inactive earlier
    */
    if (rising_edge(crtc_pins, sys->ga.crtc_pins, MC6845_HS)) {
        sys->ga.hsync_delay_counter = 3;
    }
    if (falling_edge(crtc_pins, sys->ga.crtc_pins, MC6845_HS)) {
        sys->ga.hsync_delay_counter = 0;
        sys->ga.hsync_counter = 0;
        sys->ga.sync = false;
    }
    if (sys->ga.hsync_delay_counter > 0) {
        sys->ga.hsync_delay_counter--;
        if (sys->ga.hsync_delay_counter == 0) {
            sys->ga.sync = true;
            sys->ga.hsync_counter = 5;
        }
    }
    if (sys->ga.hsync_counter > 0) {
        sys->ga.hsync_counter--;
        if (sys->ga.hsync_counter == 0) {
            sys->ga.sync = false;
        }
    }

    // FIXME delayed VSYNC to monitor

    const bool vsync = 0 != (crtc_pins & MC6845_VS);
    crt_tick(&sys->crt, sys->ga.sync, vsync);
    _cpc_ga_decode_video(sys, crtc_pins);

    sys->ga.crtc_pins = crtc_pins;

    if (sys->ga.intr) {
        cpu_pins |= Z80_INT;
    }
    return cpu_pins;
}

void _cpc_ga_decode_pixels(cpc_t* sys, uint32_t* dst, uint64_t crtc_pins) {
    /*
        compute the source address from current CRTC ma (memory address)
        and ra (raster address) like this:
    
        |ma12|ma11|ra2|ra1|ra0|ma9|ma8|...|ma2|ma1|ma0|0|
    
       Bits ma12 and m11 point to the 16 KByte page, and all
       other bits are the index into that page.
    */
    const uint16_t ma = MC6845_GET_ADDR(crtc_pins);
    const uint8_t ra = MC6845_GET_RA(crtc_pins);
    const uint32_t page_index  = (ma>>12) & 3;
    const uint32_t page_offset = ((ma & 0x03FF)<<1) | ((ra & 7)<<11);
    const uint8_t* src = &(sys->ram[page_index][page_offset]);
    uint8_t c;
    uint32_t p;
    if (0 == sys->ga.video_mode) {
        /* 160x200 @ 16 colors
           pixel    bit mask
           0:       |3|7|
           1:       |2|6|
           2:       |1|5|
           3:       |0|4|
        */
        for (int i = 0; i < 2; i++) {
            c = *src++;
            p = sys->ga.palette[((c>>7)&0x1)|((c>>2)&0x2)|((c>>3)&0x4)|((c<<2)&0x8)];
            *dst++ = p; *dst++ = p; *dst++ = p; *dst++ = p;
            p = sys->ga.palette[((c>>6)&0x1)|((c>>1)&0x2)|((c>>2)&0x4)|((c<<3)&0x8)];
            *dst++ = p; *dst++ = p; *dst++ = p; *dst++ = p;
        }
    }
    else if (1 == sys->ga.video_mode) {
        /* 320x200 @ 4 colors
           pixel    bit mask
           0:       |3|7|
           1:       |2|6|
           2:       |1|5|
           3:       |0|4|
        */
        for (int i = 0; i < 2; i++) {
            c = *src++;
            p = sys->ga.palette[((c>>2)&2)|((c>>7)&1)];
            *dst++ = p; *dst++ = p;
            p = sys->ga.palette[((c>>1)&2)|((c>>6)&1)];
            *dst++ = p; *dst++ = p;
            p = sys->ga.palette[((c>>0)&2)|((c>>5)&1)];
            *dst++ = p; *dst++ = p;
            p = sys->ga.palette[((c<<1)&2)|((c>>4)&1)];
            *dst++ = p; *dst++ = p;
        }
    }
    else if (2 == sys->ga.video_mode) {
        /* 640x200 @ 2 colors */
        for (int i = 0; i < 2; i++) {
            c = *src++;
            for (int j = 7; j >= 0; j--) {
                *dst++ = sys->ga.palette[(c>>j)&1];
            }
        }
    }
}

void _cpc_ga_decode_video(cpc_t* sys, uint64_t crtc_pins) {
    if (sys->crt.visible) {
        int dst_x = sys->crt.pos_x * 16;
        int dst_y = sys->crt.pos_y;
        uint32_t* dst = &(rgba8_buffer[dst_x + dst_y * CPC_DISPLAY_WIDTH]);
        if (crtc_pins & MC6845_DE) {
            /* decode visible pixels */
            _cpc_ga_decode_pixels(sys, dst, crtc_pins);
        }
        else if (crtc_pins & (MC6845_HS|MC6845_VS)) {
            /* during horizontal/vertical sync: blacker than black */
            for (int i = 0; i < 16; i++) {
                dst[i] = 0xFF000000;
            }
        }
        else {
            /* border color */
            for (int i = 0; i < 16; i++) {
                dst[i] = sys->ga.border_color;
            }
        }
    }
}

/* CPC SNA fileformat header: http://cpctech.cpc-live.com/docs/snapshot.html */
typedef struct {
    uint8_t magic[8];     // must be "MV - SNA"
    uint8_t pad0[8];
    uint8_t version;
    uint8_t F, A, C, B, E, D, L, H, R, I;
    uint8_t IFF1, IFF2;
    uint8_t IX_l, IX_h;
    uint8_t IY_l, IY_h;
    uint8_t SP_l, SP_h;
    uint8_t PC_l, PC_h;
    uint8_t IM;
    uint8_t F_, A_, C_, B_, E_, D_, L_, H_;
    uint8_t selected_pen;
    uint8_t pens[17];             // palette + border colors
    uint8_t gate_array_config;
    uint8_t ram_config;
    uint8_t crtc_selected;
    uint8_t crtc_regs[18];
    uint8_t rom_config;
    uint8_t ppi_a;
    uint8_t ppi_b;
    uint8_t ppi_c;
    uint8_t ppi_control;
    uint8_t psg_selected;
    uint8_t psg_regs[16];
    uint8_t dump_size_l;
    uint8_t dump_size_h;
    uint8_t pad1[0x93];
} sna_header;

bool cpc_load_sna(cpc_t* sys, const uint8_t* ptr, uint32_t num_bytes) {
    if (num_bytes < sizeof(sna_header)) {
        return false;
    }
    const sna_header* hdr = (const sna_header*) ptr;
    if ((hdr->magic[5] != 'S') || (hdr->magic[6] != 'N') || (hdr->magic[7] != 'A')) {
        return false;
    }
    ptr += sizeof(sna_header);

    /* copy 64 or 128 KByte memory dump */
    const uint16_t dump_size = hdr->dump_size_h<<8 | hdr->dump_size_l;
    const uint32_t dump_num_bytes = (dump_size == 64) ? 0x10000 : 0x20000;
    if (num_bytes > (sizeof(sna_header) + dump_num_bytes)) {
        return false;
    }
    if (dump_num_bytes > sizeof(sys->ram)) {
        return false;
    }
    memcpy(sys->ram, ptr, dump_num_bytes);

    z80_reset(&sys->cpu);
    z80_set_f(&sys->cpu, hdr->F); z80_set_a(&sys->cpu,hdr->A);
    z80_set_c(&sys->cpu, hdr->C); z80_set_b(&sys->cpu, hdr->B);
    z80_set_e(&sys->cpu, hdr->E); z80_set_d(&sys->cpu, hdr->D);
    z80_set_l(&sys->cpu, hdr->L); z80_set_h(&sys->cpu, hdr->H);
    z80_set_r(&sys->cpu, hdr->R); z80_set_i(&sys->cpu, hdr->I);
    z80_set_iff1(&sys->cpu, (hdr->IFF1 & 1) != 0);
    z80_set_iff2(&sys->cpu, (hdr->IFF2 & 1) != 0);
    z80_set_ix(&sys->cpu, hdr->IX_h<<8 | hdr->IX_l);
    z80_set_iy(&sys->cpu, hdr->IY_h<<8 | hdr->IY_l);
    z80_set_sp(&sys->cpu, hdr->SP_h<<8 | hdr->SP_l);
    z80_set_pc(&sys->cpu, hdr->PC_h<<8 | hdr->PC_l);
    z80_set_im(&sys->cpu, hdr->IM);
    z80_set_af_(&sys->cpu, hdr->A_<<8 | hdr->F_);
    z80_set_bc_(&sys->cpu, hdr->B_<<8 | hdr->C_);
    z80_set_de_(&sys->cpu, hdr->D_<<8 | hdr->E_);
    z80_set_hl_(&sys->cpu, hdr->H_<<8 | hdr->L_);

    for (int i = 0; i < 16; i++) {
        sys->ga.palette[i] = sys->ga.colors[hdr->pens[i] & 0x1F];
    }
    sys->ga.border_color = sys->ga.colors[hdr->pens[16] & 0x1F];
    sys->ga.pen = hdr->selected_pen & 0x1F;
    sys->ga.config = hdr->gate_array_config & 0x3F;
    sys->ga.next_video_mode = hdr->gate_array_config & 3;
    sys->ga.ram_config = hdr->ram_config & 0x3F;
    sys->upper_rom_select = hdr->rom_config;
    _cpc_update_memory_mapping(sys);

    for (int i = 0; i < 18; i++) {
        sys->vdg.reg[i] = hdr->crtc_regs[i];
    }
    sys->vdg.sel = hdr->crtc_selected;

    sys->ppi.output[I8255_PORT_A] = hdr->ppi_a;
    sys->ppi.output[I8255_PORT_B] = hdr->ppi_b;
    sys->ppi.output[I8255_PORT_C] = hdr->ppi_c;
    sys->ppi.control = hdr->ppi_control;

    for (int i = 0; i < 16; i++) {
        ay38910_iorq(&sys->psg, AY38910_BDIR|AY38910_BC1|(i<<16));
        ay38910_iorq(&sys->psg, AY38910_BDIR|(hdr->psg_regs[i]<<16));
    }
    ay38910_iorq(&sys->psg, AY38910_BDIR|AY38910_BC1|(hdr->psg_selected<<16));
    return true;
}

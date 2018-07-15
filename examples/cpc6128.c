/*
    cpc.c

    Amstrad CPC 6128. No tape or disc emulation, audio is emulated but
    not output.
*/
#include "sokol_app.h"
#include "sokol_time.h"
#include "sokol_audio.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/i8255.h"
#include "chips/mc6845.h"
#include "chips/crt.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "roms/cpc-roms.h"
#include "common/gfx.h"
#include "common/fs.h"

/* CPC 6128 emulator state and callbacks */
#define CPC_FREQ (4000000)
#define CPC_DISP_WIDTH (768)
#define CPC_DISP_HEIGHT (272)
#define CPC_SAMPLE_BUFFER_SIZE (32)
typedef struct {
    z80_t cpu;
    ay38910_t psg;
    mc6845_t vdg;
    i8255_t ppi;

    bool joy_enabled;
    uint8_t joy_mask;
    uint32_t tick_count;
    uint8_t upper_rom_select;

    uint8_t ga_config;              // out to port 0x7Fxx func 0x80
    uint8_t ga_next_video_mode;
    uint8_t ga_video_mode;
    uint8_t ga_ram_config;          // out to port 0x7Fxx func 0xC0
    uint8_t ga_pen;                 // currently selected pen (or border)
    uint32_t ga_palette[16];        // the current pen colors
    uint32_t ga_border_color;       // the current border color
    int ga_hsync_irq_counter;       // incremented each scanline, reset at 52
    int ga_hsync_after_vsync_counter;   // for 2-hsync-delay after vsync
    int ga_hsync_delay_counter;     // hsync to monitor is delayed 2 ticks
    int ga_hsync_counter;           // countdown until hsync to monitor is deactivated
    bool ga_sync;                   // gate-array generated video sync (modified HSYNC)
    bool ga_int;                    // GA interrupt pin active
    uint64_t ga_crtc_pins;          // store CRTC pins to detect rising/falling bits

    crt_t crt;
    kbd_t kbd;
    mem_t mem;
    int sample_pos;
    float sample_buffer[CPC_SAMPLE_BUFFER_SIZE];
    uint8_t ram[8][0x4000];
} cpc_t;
cpc_t cpc;

uint32_t frame_count;
uint32_t overrun_ticks;
uint64_t last_time_stamp;

/*
    the fixed hardware color palette

    http://www.cpcwiki.eu/index.php/CPC_Palette
    http://www.grimware.org/doku.php/documentations/devices/gatearray

    index into this palette is the 'hardware color number' & 0x1F
    order is ABGR
*/
const uint32_t cpc_colors[32] = {
    0xff6B7D6E,         // #40 white
    0xff6D7D6E,         // #41 white
    0xff6BF300,         // #42 sea green
    0xff6DF3F3,         // #43 pastel yellow
    0xff6B0200,         // #44 blue
    0xff6802F0,         // #45 purple
    0xff687800,         // #46 cyan
    0xff6B7DF3,         // #47 pink
    0xff6802F3,         // #48 purple
    0xff6BF3F3,         // #49 pastel yellow
    0xff0DF3F3,         // #4A bright yellow
    0xffF9F3FF,         // #4B bright white
    0xff0605F3,         // #4C bright red
    0xffF402F3,         // #4D bright magenta
    0xff0D7DF3,         // #4E orange
    0xffF980FA,         // #4F pastel magenta
    0xff680200,         // #50 blue
    0xff6BF302,         // #51 sea green
    0xff01F002,         // #52 bright green
    0xffF2F30F,         // #53 bright cyan
    0xff010200,         // #54 black
    0xffF4020C,         // #55 bright blue
    0xff017802,         // #56 green
    0xffF47B0C,         // #57 sky blue
    0xff680269,         // #58 magenta
    0xff6BF371,         // #59 pastel green
    0xff04F571,         // #5A lime
    0xffF4F371,         // #5B pastel cyan
    0xff01026C,         // #5C red
    0xffF2026C,         // #5D mauve
    0xff017B6E,         // #5E yellow
    0xffF67B6E,         // #5F pastel blue
};

void cpc_init(void);
void cpc_init_keymap(void);
void cpc_update_memory_mapping(void);
uint64_t cpc_cpu_tick(int num_ticks, uint64_t pins);
uint64_t cpc_cpu_iorq(uint64_t pins);
uint64_t cpc_ppi_out(int port_id, uint64_t pins, uint8_t data);
uint8_t cpc_ppi_in(int port_id);
void cpc_psg_out(int port_id, uint8_t data);
uint8_t cpc_psg_in(int port_id);
uint64_t cpc_ga_tick(uint64_t pins);
void cpc_ga_int_ack();
void cpc_ga_decode_video(uint64_t crtc_pins);
void cpc_ga_decode_pixels(uint32_t* dst, uint64_t crtc_pins);
bool cpc_load_sna(const uint8_t* ptr, uint32_t num_bytes);

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    fs_init();
    if (argc > 1) {
        const char* path = argv[1];
        fs_load_file(path);
    }
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = CPC_DISP_WIDTH,
        .height = 2 * CPC_DISP_HEIGHT,
        .window_title = "CPC 6128",
        .ios_keyboard_resizes_canvas = true
    };
}

/* one-time application init */
void app_init(void) {
    gfx_init(CPC_DISP_WIDTH, CPC_DISP_HEIGHT, 1, 2);
    saudio_setup(&(saudio_desc){0});
    cpc_init();
    last_time_stamp = stm_now();
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame(void) {
    frame_count++;
    int32_t ticks_to_run = 0;
    /*
    if (saudio_isvalid()) {
        double audio_needed = (double)saudio_expect();
        double sample_rate = (double)saudio_sample_rate();
        ticks_to_run = (uint32_t) (((CPC_FREQ * audio_needed)/sample_rate) - overrun_ticks);
    }
    else {
    */
        /* no audio available, use normal clock */
        double frame_time_ms = stm_ms(stm_laptime(&last_time_stamp));
        if (frame_time_ms < 24.0) {
            frame_time_ms = 16.666667;
        }
        else {
            frame_time_ms = 33.333334;
        }
        ticks_to_run = (uint32_t) (((CPC_FREQ * frame_time_ms) / 1000.0) - overrun_ticks);
    //}
    if (ticks_to_run > 0) {
        int32_t ticks_executed = z80_exec(&cpc.cpu, ticks_to_run);
        assert(ticks_executed >= ticks_to_run);
        overrun_ticks = ticks_executed - ticks_to_run;
        kbd_update(&cpc.kbd);
    }
    else {
        overrun_ticks = 0;
    }
    gfx_draw();
    /* load SNA file? */
    if (fs_ptr() && frame_count > 120) {
        cpc_load_sna(fs_ptr(), fs_size());
        fs_free();
        cpc.joy_enabled = true;
    }
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if (c < KBD_MAX_KEYS) {
                kbd_key_down(&cpc.kbd, c);
                kbd_key_up(&cpc.kbd, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_SPACE:        c = cpc.joy_enabled ? 0 : 0x20; break;
                case SAPP_KEYCODE_LEFT:         c = cpc.joy_enabled ? 0 : 0x08; break;
                case SAPP_KEYCODE_RIGHT:        c = cpc.joy_enabled ? 0 : 0x09; break;
                case SAPP_KEYCODE_DOWN:         c = cpc.joy_enabled ? 0 : 0x0A; break;
                case SAPP_KEYCODE_UP:           c = cpc.joy_enabled ? 0 : 0x0B; break;
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
                    kbd_key_down(&cpc.kbd, c);
                }
                else {
                    kbd_key_up(&cpc.kbd, c);
                }
            }
            else if (cpc.joy_enabled) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    switch (event->key_code) {
                        case SAPP_KEYCODE_SPACE:        cpc.joy_mask |= 1<<4; break;
                        case SAPP_KEYCODE_LEFT:         cpc.joy_mask |= 1<<2; break;
                        case SAPP_KEYCODE_RIGHT:        cpc.joy_mask |= 1<<3; break;
                        case SAPP_KEYCODE_DOWN:         cpc.joy_mask |= 1<<1; break;
                        case SAPP_KEYCODE_UP:           cpc.joy_mask |= 1<<0; break;
                        default: break;
                    }
                }
                else {
                    switch (event->key_code) {
                        case SAPP_KEYCODE_SPACE:        cpc.joy_mask &= ~(1<<4); break;
                        case SAPP_KEYCODE_LEFT:         cpc.joy_mask &= ~(1<<2); break;
                        case SAPP_KEYCODE_RIGHT:        cpc.joy_mask &= ~(1<<3); break;
                        case SAPP_KEYCODE_DOWN:         cpc.joy_mask &= ~(1<<1); break;
                        case SAPP_KEYCODE_UP:           cpc.joy_mask &= ~(1<<0); break;
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
    saudio_shutdown();
    gfx_shutdown();
}

/* CPC 6128 emulator init */
void cpc_init(void) {
    cpc.upper_rom_select = 0;
    cpc.tick_count = 0;
    cpc.ga_next_video_mode = 1;
    cpc.ga_video_mode = 1;
    cpc.ga_hsync_delay_counter = 2;
    cpc_init_keymap();
    cpc_update_memory_mapping();

    z80_init(&cpc.cpu, cpc_cpu_tick);
    i8255_init(&cpc.ppi, cpc_ppi_in, cpc_ppi_out);
    mc6845_init(&cpc.vdg, MC6845_TYPE_UM6845R);
    crt_init(&cpc.crt, CRT_PAL, 6, 32, CPC_DISP_WIDTH/16, CPC_DISP_HEIGHT);
    ay38910_init(&cpc.psg, &(ay38910_desc_t){
        .type = AY38910_TYPE_8912,
        .in_cb = cpc_psg_in,
        .out_cb = cpc_psg_out,
        .tick_hz = 1000000,
        .sound_hz = 44100,
        .magnitude = 0.7f
    });

    /* CPU start address */
    cpc.cpu.state.PC = 0x0000;
}

void cpc_init_keymap() {
    /*
        http://cpctech.cpc-live.com/docs/keyboard.html
    
        CPC has a 10 columns by 8 lines keyboard matrix. The 10 columns
        are lit up by bits 0..3 of PPI port C connected to a 74LS145
        BCD decoder, and the lines are read through port A of the
        AY-3-8910 chip.
    */
    kbd_init(&cpc.kbd, 1);
    const char* keymap =
        /* no shift */
        "   ^08641 "
        "  [-97532 "
        "   @oure  "
        "  ]piytwq "
        "   ;lhgs  "
        "   :kjfda "
        "  \\/mnbc  "
        "   ., vxz "

        /* shift */
        "    _(&$! "
        "  {=)'%#\" "
        "   |OURE  "
        "  }PIYTWQ "
        "   +LHGS  "
        "   *KJFDA "
        "  `?MNBC  "
        "   >< VXZ ";
    /* shift key is on column 2, line 5 */
    kbd_register_modifier(&cpc.kbd, 0, 2, 5);
    /* ctrl key is on column 2, line 7 */
    kbd_register_modifier(&cpc.kbd, 1, 2, 7);

    for (int shift = 0; shift < 2; shift++) {
        for (int col = 0; col < 10; col++) {
            for (int line = 0; line < 8; line++) {
                int c = keymap[shift*80 + line*10 + col];
                if (c != 0x20) {
                    kbd_register_key(&cpc.kbd, c, col, line, shift?(1<<0):0);
                }
            }
        }
    }

    /* special keys */
    kbd_register_key(&cpc.kbd, 0x20, 5, 7, 0);    // space
    kbd_register_key(&cpc.kbd, 0x08, 1, 0, 0);    // cursor left
    kbd_register_key(&cpc.kbd, 0x09, 0, 1, 0);    // cursor right
    kbd_register_key(&cpc.kbd, 0x0A, 0, 2, 0);    // cursor down
    kbd_register_key(&cpc.kbd, 0x0B, 0, 0, 0);    // cursor up
    kbd_register_key(&cpc.kbd, 0x01, 9, 7, 0);    // delete
    kbd_register_key(&cpc.kbd, 0x0C, 2, 0, 0);    // clr
    kbd_register_key(&cpc.kbd, 0x0D, 2, 2, 0);    // return
    kbd_register_key(&cpc.kbd, 0x03, 8, 2, 0);    // escape
}

static const int cpc_ram_config_table[8][4] = {
    { 0, 1, 2, 3 },
    { 0, 1, 2, 7 },
    { 4, 5, 6, 7 },
    { 0, 3, 2, 7 },
    { 0, 4, 2, 3 },
    { 0, 5, 2, 3 },
    { 0, 6, 2, 3 },
    { 0, 7, 2, 3 },
};

void cpc_update_memory_mapping() {
    /* index into RAM config array */
    int ram_table_index = cpc.ga_ram_config & 0x07;
    const uint8_t* rom0_ptr = dump_cpc6128_os;
    const uint8_t* rom1_ptr;
    if (cpc.upper_rom_select == 7) {
        rom1_ptr = dump_cpc6128_amsdos;
    }
    else {
        rom1_ptr = dump_cpc6128_basic;
    }
    const int i0 = cpc_ram_config_table[ram_table_index][0];
    const int i1 = cpc_ram_config_table[ram_table_index][1];
    const int i2 = cpc_ram_config_table[ram_table_index][2];
    const int i3 = cpc_ram_config_table[ram_table_index][3];
    /* 0x0000..0x3FFF */
    if (cpc.ga_config & (1<<2)) {
        /* read/write from and to RAM bank */
        mem_map_ram(&cpc.mem, 0, 0x0000, 0x4000, cpc.ram[i0]);
    }
    else {
        /* read from ROM, write to RAM */
        mem_map_rw(&cpc.mem, 0, 0x0000, 0x4000, rom0_ptr, cpc.ram[i0]);
    }
    /* 0x4000..0x7FFF */
    mem_map_ram(&cpc.mem, 0, 0x4000, 0x4000, cpc.ram[i1]);
    /* 0x8000..0xBFFF */
    mem_map_ram(&cpc.mem, 0, 0x8000, 0x4000, cpc.ram[i2]);
    /* 0xC000..0xFFFF */
    if (cpc.ga_config & (1<<3)) {
        /* read/write from and to RAM bank */
        mem_map_ram(&cpc.mem, 0, 0xC000, 0x4000, cpc.ram[i3]);
    }
    else {
        /* read from ROM, write to RAM */
        mem_map_rw(&cpc.mem, 0, 0xC000, 0x4000, rom1_ptr, cpc.ram[i3]);
    }
}

uint64_t cpc_cpu_tick(int num_ticks, uint64_t pins) {
    /* interrupt acknowledge? */
    if ((pins & (Z80_M1|Z80_IORQ)) == (Z80_M1|Z80_IORQ)) {
        cpc_ga_int_ack();
    }

    /* memory and IO requests */
    if (pins & Z80_MREQ) {
        /* CPU MEMORY REQUEST */
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem_rd(&cpc.mem, addr));
        }
        else if (pins & Z80_WR) {
            mem_wr(&cpc.mem, addr, Z80_GET_DATA(pins));
        }
    }
    else if ((pins & Z80_IORQ) && (pins & (Z80_RD|Z80_WR))) {
        /* CPU IO REQUEST */
        pins = cpc_cpu_iorq(pins);
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
            bool wait_pin = (cpc.tick_count++ & 3) != 0;
            wait = (wait_pin && (wait || (i == wait_scan_tick)));
            if (wait) {
                wait_cycles++;
            }
            /* on every 4th clock cycle, tick the system */
            if (!wait_pin) {
                if (ay38910_tick(&cpc.psg)) {
                    cpc.sample_buffer[cpc.sample_pos++] = cpc.psg.sample;
                    if (cpc.sample_pos == CPC_SAMPLE_BUFFER_SIZE) {
                        cpc.sample_pos = 0;
                        saudio_push(cpc.sample_buffer, CPC_SAMPLE_BUFFER_SIZE);
                    }
                }
                pins = cpc_ga_tick(pins);
            }
        }
        while (wait);
    }
    Z80_SET_WAIT(pins, wait_cycles);

    return pins;
}

uint64_t cpc_cpu_iorq(uint64_t pins) {
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
        pins = i8255_iorq(&cpc.ppi, ppi_pins) & Z80_PIN_MASK;
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
        pins = mc6845_iorq(&cpc.vdg, vdg_pins) & Z80_PIN_MASK;
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
                cpc.ga_pen = data & 0x1F;
                break;
            case (1<<6):
                /* select color for border or selected pen: */
                if (cpc.ga_pen & (1<<4)) {
                    /* border color */
                    cpc.ga_border_color = cpc_colors[data & 0x1F];
                }
                else {
                    cpc.ga_palette[cpc.ga_pen & 0x0F] = cpc_colors[data & 0x1F];
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
                cpc.ga_config = data;
                cpc.ga_next_video_mode = data & 3;
                if (data & (1<<4)) {
                    cpc.ga_hsync_irq_counter = 0;
                    cpc.ga_int = false;
                }
                cpc_update_memory_mapping();
                break;
            case (1<<7)|(1<<6):
                /* RAM memory management (only CPC6128) */
                cpc.ga_ram_config = data;
                cpc_update_memory_mapping();
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
        cpc.upper_rom_select = Z80_GET_DATA(pins);
        cpc_update_memory_mapping();
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

uint64_t cpc_ppi_out(int port_id, uint64_t pins, uint8_t data) {
    /*
        i8255 PPI to AY-3-8912 PSG pin connections:
            PA0..PA7    -> D0..D7
                 PC7    -> BDIR
                 PC6    -> BC1
    */
    if ((I8255_PORT_A == port_id) || (I8255_PORT_C == port_id)) {
        const uint8_t ay_ctrl = cpc.ppi.output[I8255_PORT_C] & ((1<<7)|(1<<6));
        if (ay_ctrl) {
            uint64_t ay_pins = 0;
            if (ay_ctrl & (1<<7)) { ay_pins |= AY38910_BDIR; }
            if (ay_ctrl & (1<<6)) { ay_pins |= AY38910_BC1; }
            const uint8_t ay_data = cpc.ppi.output[I8255_PORT_A];
            AY38910_SET_DATA(ay_pins, ay_data);
            ay38910_iorq(&cpc.psg, ay_pins);
        }
    }
    if (I8255_PORT_C == port_id) {
        // bits 0..3: select keyboard matrix line
        kbd_set_active_columns(&cpc.kbd, 1<<(data & 0x0F));

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

uint8_t cpc_ppi_in(int port_id) {
    if (I8255_PORT_A == port_id) {
        /* AY-3-8912 PSG function (indirectly this may also trigger
            a read of the keyboard matrix via the AY's IO port
        */
        uint64_t ay_pins = 0;
        uint8_t ay_ctrl = cpc.ppi.output[I8255_PORT_C];
        if (ay_ctrl & (1<<7)) ay_pins |= AY38910_BDIR;
        if (ay_ctrl & (1<<6)) ay_pins |= AY38910_BC1;
        uint8_t ay_data = cpc.ppi.output[I8255_PORT_A];
        AY38910_SET_DATA(ay_pins, ay_data);
        ay_pins = ay38910_iorq(&cpc.psg, ay_pins);
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
        if (cpc.vdg.vs) {
            val |= (1<<0);
        }
        return val;
    }
    else {
        /* shouldn't happen */
        return 0xFF;
    }
}

void cpc_psg_out(int port_id, uint8_t data) {
    /* this shouldn't be called */
}

uint8_t cpc_psg_in(int port_id) {
    /* read the keyboard matrix and joystick port */
    if (port_id == AY38910_PORT_A) {
        uint8_t data = (uint8_t) kbd_scan_lines(&cpc.kbd);
        if (cpc.kbd.active_columns & (1<<9)) {
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
            data |= cpc.joy_mask;
        }
        return ~data;
    }
    else {
        /* this shouldn't happen since the AY-3-8912 only has one IO port */
        return 0xFF;
    }
}

void cpc_ga_int_ack() {
    /* on interrupt acknowledge from the CPU, clear the top bit from the
        hsync counter, so the next interrupt can't occur closer then 
        32 HSYNC, and clear the gate array interrupt pin state
    */
    cpc.ga_hsync_irq_counter &= 0x1F;
    cpc.ga_int = false;
}

static bool falling_edge(uint64_t new_pins, uint64_t old_pins, uint64_t mask) {
    return 0 != (mask & (~new_pins & (new_pins ^ old_pins)));
}

static bool rising_edge(uint64_t new_pins, uint64_t old_pins, uint64_t mask) {
    return 0 != (mask & (new_pins & (new_pins ^ old_pins)));
}

uint64_t cpc_ga_tick(uint64_t cpu_pins) {
    /*
        http://cpctech.cpc-live.com/docs/ints.html
        http://www.cpcwiki.eu/forum/programming/frame-flyback-and-interrupts/msg25106/#msg25106
        https://web.archive.org/web/20170612081209/http://www.grimware.org/doku.php/documentations/devices/gatearray
    */
    uint64_t crtc_pins = mc6845_tick(&cpc.vdg);

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
    if (rising_edge(crtc_pins, cpc.ga_crtc_pins, MC6845_VS)) {
        cpc.ga_hsync_after_vsync_counter = 2;
    }
    if (falling_edge(crtc_pins, cpc.ga_crtc_pins, MC6845_HS)) {
        cpc.ga_video_mode = cpc.ga_next_video_mode;
        cpc.ga_hsync_irq_counter = (cpc.ga_hsync_irq_counter + 1) & 0x3F;

        /* 2 HSync delay? */
        if (cpc.ga_hsync_after_vsync_counter > 0) {
            cpc.ga_hsync_after_vsync_counter--;
            if (cpc.ga_hsync_after_vsync_counter == 0) {
                if (cpc.ga_hsync_irq_counter >= 32) {
                    cpc.ga_int = true;
                }
                cpc.ga_hsync_irq_counter = 0;
            }
        }
        /* normal behaviour, request interrupt each 52 scanlines */
        if (cpc.ga_hsync_irq_counter == 52) {
            cpc.ga_hsync_irq_counter = 0;
            cpc.ga_int = true;
        }
    }

    /* generate HSYNC signal to monitor:
        - starts 2 ticks after HSYNC rising edge from CRTC
        - stays active for 4 ticks or less if CRTC HSYNC goes inactive earlier
    */
    if (rising_edge(crtc_pins, cpc.ga_crtc_pins, MC6845_HS)) {
        cpc.ga_hsync_delay_counter = 3;
    }
    if (falling_edge(crtc_pins, cpc.ga_crtc_pins, MC6845_HS)) {
        cpc.ga_hsync_delay_counter = 0;
        cpc.ga_hsync_counter = 0;
        cpc.ga_sync = false;
    }
    if (cpc.ga_hsync_delay_counter > 0) {
        cpc.ga_hsync_delay_counter--;
        if (cpc.ga_hsync_delay_counter == 0) {
            cpc.ga_sync = true;
            cpc.ga_hsync_counter = 5;
        }
    }
    if (cpc.ga_hsync_counter > 0) {
        cpc.ga_hsync_counter--;
        if (cpc.ga_hsync_counter == 0) {
            cpc.ga_sync = false;
        }
    }

    // FIXME delayed VSYNC to monitor

    const bool vsync = 0 != (crtc_pins & MC6845_VS);
    crt_tick(&cpc.crt, cpc.ga_sync, vsync);
    cpc_ga_decode_video(crtc_pins);

    cpc.ga_crtc_pins = crtc_pins;

    if (cpc.ga_int) {
        cpu_pins |= Z80_INT;
    }
    return cpu_pins;
}

void cpc_ga_decode_pixels(uint32_t* dst, uint64_t crtc_pins) {
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
    const uint8_t* src = &(cpc.ram[page_index][page_offset]);
    uint8_t c;
    uint32_t p;
    if (0 == cpc.ga_video_mode) {
        /* 160x200 @ 16 colors
           pixel    bit mask
           0:       |3|7|
           1:       |2|6|
           2:       |1|5|
           3:       |0|4|
        */
        for (int i = 0; i < 2; i++) {
            c = *src++;
            p = cpc.ga_palette[((c>>7)&0x1)|((c>>2)&0x2)|((c>>3)&0x4)|((c<<2)&0x8)];
            *dst++ = p; *dst++ = p; *dst++ = p; *dst++ = p;
            p = cpc.ga_palette[((c>>6)&0x1)|((c>>1)&0x2)|((c>>2)&0x4)|((c<<3)&0x8)];
            *dst++ = p; *dst++ = p; *dst++ = p; *dst++ = p;
        }
    }
    else if (1 == cpc.ga_video_mode) {
        /* 320x200 @ 4 colors
           pixel    bit mask
           0:       |3|7|
           1:       |2|6|
           2:       |1|5|
           3:       |0|4|
        */
        for (int i = 0; i < 2; i++) {
            c = *src++;
            p = cpc.ga_palette[((c>>2)&2)|((c>>7)&1)];
            *dst++ = p; *dst++ = p;
            p = cpc.ga_palette[((c>>1)&2)|((c>>6)&1)];
            *dst++ = p; *dst++ = p;
            p = cpc.ga_palette[((c>>0)&2)|((c>>5)&1)];
            *dst++ = p; *dst++ = p;
            p = cpc.ga_palette[((c<<1)&2)|((c>>4)&1)];
            *dst++ = p; *dst++ = p;
        }
    }
    else if (2 == cpc.ga_video_mode) {
        /* 640x200 @ 2 colors */
        for (int i = 0; i < 2; i++) {
            c = *src++;
            for (int j = 7; j >= 0; j--) {
                *dst++ = cpc.ga_palette[(c>>j)&1];
            }
        }
    }
}

void cpc_ga_decode_video(uint64_t crtc_pins) {
    if (cpc.crt.visible) {
        int dst_x = cpc.crt.pos_x * 16;
        int dst_y = cpc.crt.pos_y;
        uint32_t* dst = &(rgba8_buffer[dst_x + dst_y * CPC_DISP_WIDTH]);
        if (crtc_pins & MC6845_DE) {
            /* decode visible pixels */
            cpc_ga_decode_pixels(dst, crtc_pins);
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
                dst[i] = cpc.ga_border_color;
            }
        }
    }
}

// CPC SNA fileformat header: http://cpctech.cpc-live.com/docs/snapshot.html
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

bool cpc_load_sna(const uint8_t* ptr, uint32_t num_bytes) {
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
    if (dump_num_bytes > sizeof(cpc.ram)) {
        return false;
    }
    memcpy(cpc.ram, ptr, dump_num_bytes);

    cpc.cpu.state.F = hdr->F; cpc.cpu.state.A = hdr->A;
    cpc.cpu.state.C = hdr->C; cpc.cpu.state.B = hdr->B;
    cpc.cpu.state.E = hdr->E; cpc.cpu.state.D = hdr->D;
    cpc.cpu.state.L = hdr->L; cpc.cpu.state.H = hdr->H;
    cpc.cpu.state.R = hdr->R; cpc.cpu.state.I = hdr->I;
    cpc.cpu.state.IFF1 = (hdr->IFF1 & 1) != 0;
    cpc.cpu.state.IFF2 = (hdr->IFF2 & 1) != 0;
    cpc.cpu.state.IX = hdr->IX_h<<8 | hdr->IX_l;
    cpc.cpu.state.IY = hdr->IY_h<<8 | hdr->IY_l;
    cpc.cpu.state.SP = hdr->SP_h<<8 | hdr->SP_l;
    cpc.cpu.state.PC = hdr->PC_h<<8 | hdr->PC_l;
    cpc.cpu.state.IM = hdr->IM;
    cpc.cpu.state.AF_ = hdr->A_<<8 | hdr->F_;
    cpc.cpu.state.BC_ = hdr->B_<<8 | hdr->C_;
    cpc.cpu.state.DE_ = hdr->D_<<8 | hdr->E_;
    cpc.cpu.state.HL_ = hdr->H_<<8 | hdr->L_;

    for (int i = 0; i < 16; i++) {
        cpc.ga_palette[i] = cpc_colors[hdr->pens[i] & 0x1F];
    }
    cpc.ga_border_color = cpc_colors[hdr->pens[16] & 0x1F];
    cpc.ga_pen = hdr->selected_pen & 0x1F;
    cpc.ga_config = hdr->gate_array_config & 0x3F;
    cpc.ga_next_video_mode = hdr->gate_array_config & 3;
    cpc.ga_ram_config = hdr->ram_config & 0x3F;
    cpc.upper_rom_select = hdr->rom_config;
    cpc_update_memory_mapping();

    for (int i = 0; i < 18; i++) {
        cpc.vdg.reg[i] = hdr->crtc_regs[i];
    }
    cpc.vdg.sel = hdr->crtc_selected;

    cpc.ppi.output[I8255_PORT_A] = hdr->ppi_a;
    cpc.ppi.output[I8255_PORT_B] = hdr->ppi_b;
    cpc.ppi.output[I8255_PORT_C] = hdr->ppi_c;
    cpc.ppi.control = hdr->ppi_control;

    for (int i = 0; i < 16; i++) {
        ay38910_iorq(&cpc.psg, AY38910_BDIR|AY38910_BC1|(i<<16));
        ay38910_iorq(&cpc.psg, AY38910_BDIR|(hdr->psg_regs[i]<<16));
    }
    ay38910_iorq(&cpc.psg, AY38910_BDIR|AY38910_BC1|(hdr->psg_selected<<16));
    return true;
}

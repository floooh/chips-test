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
#define SOKOL_IMPL
#include "sokol_time.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "flextgl/flextGL.h"
#define SOKOL_GLCORE33
#include "sokol_gfx.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/mc6847.h"
#include "chips/i8255.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "roms/atom-roms.h"
#include <ctype.h> /* isupper, islower, toupper, tolower */

/* the Acorn Atom hardware */
#define ATOM_FREQ (1000000)
m6502_t cpu;
mc6847_t vdg;
i8255_t ppi;
kbd_t kbd;
mem_t mem;
int counter_2_4khz = 0;
int period_2_4khz = 0;
bool state_2_4khz = false;
uint8_t ram[1<<16];     /* only 40 KByte used */

/* emulator callbacks */
uint64_t cpu_tick(uint64_t pins);
uint64_t vdg_fetch(uint64_t pins);
uint8_t ppi_in(int port_id);
uint64_t ppi_out(int port_id, uint64_t pins, uint8_t data);

/* GLFW keyboard input callbacks */
void on_key(GLFWwindow* w, int key, int scancode, int action, int mods);
void on_char(GLFWwindow* w, unsigned int codepoint);

/* host rendering functions and resources */
GLFWwindow* gfx_init();
void gfx_draw(GLFWwindow* w);
void gfx_shutdown();
sg_draw_state draw_state;
uint32_t rgba8_buffer[MC6847_DISPLAY_WIDTH * MC6847_DISPLAY_HEIGHT];

/* xorshift randomness for memory initialization */
uint32_t xorshift_state = 0x6D98302B;
uint32_t xorshift32() {
    uint32_t x = xorshift_state;
    x ^= x<<13; x ^= x>>17; x ^= x<<5;
    xorshift_state = x;
    return x;
}

int main() {
    /* setup rendering via GLFW and sokol_gfx */
    GLFWwindow* w = gfx_init();

    /* setup memory map, first fill memory with random values */
    for (int i = 0; i < (int)sizeof(ram);) {
        uint32_t r = xorshift32();
        ram[i++]=r>>24; ram[i++]=r>>16; ram[i++]=r>>8; ram[i++]=r;
    }
    mem_init(&mem);
    /* 32 KByte RAM + 8 KByte vidmem */
    mem_map_ram(&mem, 0, 0x0000, 0xA000, ram);
    /* hole in 0xA000 to 0xAFFF for utility roms */
    /* 0xB000 to 0xBFFF is memory-mapped IO area (not mapped to host memory) */
    /* 0xC000 to 0xFFFF are operating system roms */
    mem_map_rom(&mem, 0, 0xC000, 0x1000, dump_abasic);
    mem_map_rom(&mem, 0, 0xD000, 0x1000, dump_afloat);
    mem_map_rom(&mem, 0, 0xE000, 0x1000, dump_dosrom);
    mem_map_rom(&mem, 0, 0xF000, 0x1000, dump_abasic+0x1000);

    /*  setup the keyboard matrix
        the Atom has a 10x8 keyboard matrix, where the
        entire line 6 is for the Ctrl key, and the entire
        line 7 is the Shift key
    */
    kbd_init(&kbd, 1);
    /* shift key is entire line 7 */
    const int shift = (1<<0); kbd_register_modifier_line(&kbd, 0, 7);
    /* ctrl key is entire line 6 */
    const int ctrl = (1<<1); kbd_register_modifier_line(&kbd, 1, 6);
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
                    kbd_register_key(&kbd, c, col, line, layer?shift:0);
                }
            }
        }
    }
    /* special keys */
    kbd_register_key(&kbd, 0x20, 9, 0, 0);      /* space */
    kbd_register_key(&kbd, 0x01, 4, 1, 0);      /* backspace */
    kbd_register_key(&kbd, 0x08, 3, 0, shift);  /* left */
    kbd_register_key(&kbd, 0x09, 3, 0, 0);      /* right */
    kbd_register_key(&kbd, 0x0A, 2, 0, shift);  /* down */
    kbd_register_key(&kbd, 0x0B, 2, 0, 0);      /* up */
    kbd_register_key(&kbd, 0x0D, 6, 1, 0);      /* return/enter */
    kbd_register_key(&kbd, 0x1B, 0, 5, 0);      /* escape */
    kbd_register_key(&kbd, 0x0C, 5, 4, ctrl);   /* Ctrl+L, clear screen, mapped to F1 */

    /* initialize chips */
    m6502_init(&cpu, cpu_tick);
    mc6847_desc_t vdg_desc = {
        .tick_hz = ATOM_FREQ,
        .rgba8_buffer = rgba8_buffer,
        .rgba8_buffer_size = sizeof(rgba8_buffer),
        .fetch_cb = vdg_fetch
    };
    mc6847_init(&vdg, &vdg_desc);
    i8255_init(&ppi, ppi_in, ppi_out);

    /* initialize 2.4 khz counter */
    period_2_4khz = ATOM_FREQ / 2400;
    counter_2_4khz = 0;
    state_2_4khz = false;

    /* GLFW keyboard callbacks */
    glfwSetKeyCallback(w, on_key);
    glfwSetCharCallback(w, on_char);

    /* reset the CPU to go into 'start state' */
    m6502_reset(&cpu);

    /* emulate and draw frames */
    uint32_t overrun_ticks = 0;
    uint64_t last_time_stamp = stm_now();
    while (!glfwWindowShouldClose(w)) {
        /* number of 2MHz ticks in host frame */
        double frame_time = stm_sec(stm_laptime(&last_time_stamp));
        uint32_t ticks_to_run = (uint32_t) ((ATOM_FREQ * frame_time) - overrun_ticks);
        uint32_t ticks_executed = m6502_exec(&cpu, ticks_to_run);
        assert(ticks_executed >= ticks_to_run);
        overrun_ticks = ticks_executed - ticks_to_run;
        kbd_update(&kbd);
        gfx_draw(w);
    }
    /* shutdown */
    gfx_shutdown();
    return 0;
}

/* CPU tick callback */
uint64_t cpu_tick(uint64_t pins) {

    /* tick the video chip */
    mc6847_tick(&vdg);

    /* tick the 2.4khz counter */
    counter_2_4khz++;
    if (counter_2_4khz >= period_2_4khz) {
        state_2_4khz = !state_2_4khz;
        counter_2_4khz -= period_2_4khz;
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
            pins = i8255_iorq(&ppi, ppi_pins) & M6502_PIN_MASK;
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
            M6502_SET_DATA(pins, mem_rd(&mem, addr));
        }
        else {
            /* memory access */
            mem_wr(&mem, addr, M6502_GET_DATA(pins));
        }
    }
    return pins;
}

/* video memory fetch callback */
uint64_t vdg_fetch(uint64_t pins) {
    const uint16_t addr = MC6847_GET_ADDR(pins);
    uint8_t data = ram[(addr + 0x8000) & 0xFFFF];
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
uint64_t ppi_out(int port_id, uint64_t pins, uint8_t data) {
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
        kbd_set_active_columns(&kbd, 1<<(data & 0x0F));
        uint64_t vdg_pins = 0;
        uint64_t vdg_mask = MC6847_AG|MC6847_GM0|MC6847_GM1|MC6847_GM2;
        if (data & (1<<4)) { vdg_pins |= MC6847_AG; }
        if (data & (1<<5)) { vdg_pins |= MC6847_GM0; }
        if (data & (1<<6)) { vdg_pins |= MC6847_GM1; }
        if (data & (1<<7)) { vdg_pins |= MC6847_GM2; }
        mc6847_ctrl(&vdg, vdg_pins, vdg_mask);
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
        mc6847_ctrl(&vdg, vdg_pins, vdg_mask);
    }
    return pins;
}

/* i8255 PPI input callback */
uint8_t ppi_in(int port_id) {
    uint8_t data = 0;
    if (I8255_PORT_B == port_id) {
        /* keyboard row state */
        data = ~kbd_scan_lines(&kbd);
    }
    else if (I8255_PORT_C == port_id) {
        /*  PPI port C input:
            4:  input: 2400 Hz
            5:  input: cassette
            6:  input: keyboard repeat
            7:  input: MC6847 FSYNC

            NOTE: only the 2400 Hz oscillator and FSYNC pins is emulated here
        */
        if (state_2_4khz) {
            data |= (1<<4);
        }
        /* FIXME: always send REPEAT key as 'not pressed' */
        data |= (1<<6);
        /* vblank pin (cleared during vblank) */
        if (0 == (vdg.pins & MC6847_FS)) {
            data |= (1<<7);
        }
    }
    return data;
}

/* GLFW character callback */
void on_char(GLFWwindow* w, unsigned int codepoint) {
    if (codepoint < KBD_MAX_KEYS) {
        /* need to invert case (unshifted is upper caps, shifted is lower caps */
        if (isupper(codepoint)) {
            codepoint = tolower(codepoint);
        }
        else if (islower(codepoint)) {
            codepoint = toupper(codepoint);
        }
        kbd_key_down(&kbd, (int)codepoint);
        kbd_key_up(&kbd, (int)codepoint);
    }
}

/* GLFW 'raw key' callback */
void on_key(GLFWwindow* w, int glfw_key, int scancode, int action, int mods) {
    int key = 0;
    switch (glfw_key) {
        case GLFW_KEY_ENTER:        key = 0x0D; break;
        case GLFW_KEY_RIGHT:        key = 0x09; break;
        case GLFW_KEY_LEFT:         key = 0x08; break;
        case GLFW_KEY_DOWN:         key = 0x0A; break;
        case GLFW_KEY_UP:           key = 0x0B; break;
        case GLFW_KEY_INSERT:       key = 0x1A; break;
        case GLFW_KEY_HOME:         key = 0x19; break;
        case GLFW_KEY_BACKSPACE:    key = 0x01; break;
        case GLFW_KEY_ESCAPE:       key = 0x1B; break;
        case GLFW_KEY_F1:           key = 0x0C; break; /* mapped to Ctrl+L (clear screen) */
    }
    if (key) {
        if ((GLFW_PRESS == action) || (GLFW_REPEAT == action)) {
            kbd_key_down(&kbd, key);
        }
        else if (GLFW_RELEASE == action) {
            kbd_key_up(&kbd, key);
        }
    }
}

/* setup GLFW, flextGL, sokol_gfx, sokol_time and create gfx resources */
GLFWwindow* gfx_init() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* w = glfwCreateWindow(2*MC6847_DISPLAY_WIDTH, 2*MC6847_DISPLAY_HEIGHT, "Acorn Atom Example", 0, 0);
    glfwMakeContextCurrent(w);
    glfwSwapInterval(1);
    flextInit(w);
    sg_setup(&(sg_desc){0});
    stm_setup();

    float quad_vertices[] = { 0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f };
    draw_state.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(quad_vertices),
        .content = quad_vertices,
    });
    sg_shader fsq_shd = sg_make_shader(&(sg_shader_desc){
        .fs.images = {
            [0] = { .name="tex", .type=SG_IMAGETYPE_2D },
        },
        .vs.source =
            "#version 330\n"
            "in vec2 pos;\n"
            "out vec2 uv0;\n"
            "void main() {\n"
            "  gl_Position = vec4(pos*2.0-1.0, 0.5, 1.0);\n"
            "  uv0 = vec2(pos.x, 1.0-pos.y);\n"
            "}\n",
        .fs.source =
            "#version 330\n"
            "uniform sampler2D tex0;\n"
            "in vec2 uv0;\n"
            "out vec4 frag_color;\n"
            "void main() {\n"
            "  frag_color = texture(tex0, uv0);\n"
            "}\n"
    });
    draw_state.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {
            .attrs[0] = { .name="pos", .format=SG_VERTEXFORMAT_FLOAT2 }
        },
        .shader = fsq_shd,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP
    });
    draw_state.fs_images[0] = sg_make_image(&(sg_image_desc){
        .width = MC6847_DISPLAY_WIDTH,
        .height = MC6847_DISPLAY_HEIGHT,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = SG_USAGE_STREAM,
    });
    return w;
}

/* decode emulator framebuffer into texture, and draw fullscreen rect */
void gfx_draw(GLFWwindow* w) {
    sg_update_image(draw_state.fs_images[0], &(sg_image_content){
        .subimage[0][0] = { .ptr = rgba8_buffer, .size = sizeof(rgba8_buffer) }
    });
    int width, height;
    glfwGetFramebufferSize(w, &width, &height);
    sg_pass_action pass_action = {
        .colors[0].action = SG_ACTION_DONTCARE
    };
    sg_begin_default_pass(&pass_action, width, height);
    sg_apply_draw_state(&draw_state);
    sg_draw(0, 4, 1);
    sg_end_pass();
    sg_commit();
    glfwSwapBuffers(w);
    glfwPollEvents();
}

/* shutdown sokol and GLFW */
void gfx_shutdown() {
    sg_shutdown();
    glfwTerminate();
}

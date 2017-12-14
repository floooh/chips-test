//------------------------------------------------------------------------------
//  kc87.c
//
//  Wiring diagram: http://www.sax.de/~zander/kc/kcsch_1.pdf
//  Detailed Manual: http://www.sax.de/~zander/z9001/doku/z9_fub.pdf
//
//  not emulated: beeper sound, border color, 40x20 video mode
//------------------------------------------------------------------------------
#define SOKOL_IMPL
#include "sokol_time.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "flextgl/flextGL.h"
#define SOKOL_GLCORE33
#include "sokol_gfx.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/z80ctc.h"
#include "chips/keyboard_matrix.h"
#include "roms/kc87-roms.h"
#include <ctype.h> /* isupper, islower, toupper, tolower */

/* the KC87 hardware */
#define KC87_FREQ (2457600)
z80 cpu;
z80pio pio1;
z80pio pio2;
z80ctc ctc;
keyboard_matrix kbd;
uint32_t blink_counter = 0;
bool blink_flip_flop = false;
uint64_t ctc_clktrg3_state = 0;
uint8_t mem[1<<16];
const  uint32_t palette[8] = {
    0xFF000000,     // black
    0xFF0000FF,     // red
    0xFF00FF00,     // green
    0xFF00FFFF,     // yellow
    0xFFFF0000,     // blue
    0xFFFF00FF,     // purple
    0xFFFFFF00,     // cyan
    0xFFFFFFFF,     // white
};

/* emulator callbacks */
uint64_t tick(int num, uint64_t pins);
uint8_t pio1_in(int port_id);
void pio1_out(int port_id, uint8_t data);
uint8_t pio2_in(int port_id);
void pio2_out(int port_id, uint8_t data);

/* GLFW keyboard input callbacks */
void on_key(GLFWwindow* w, int key, int scancode, int action, int mods);
void on_char(GLFWwindow* w, unsigned int codepoint);

/* rendering functions and resources */
GLFWwindow* gfx_init();
void gfx_draw(GLFWwindow* w);
void gfx_shutdown();
sg_draw_state draw_state;
uint32_t rgba8_buffer[320*192];

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

    /* initialize CPU, PIOs and CTC */
    z80_init(&cpu, &(z80_desc){ .tick_cb = tick, });
    z80pio_init(&pio1, &(z80pio_desc){ .in_cb=pio1_in, .out_cb=pio1_out });
    z80pio_init(&pio2, &(z80pio_desc){ .in_cb=pio2_in, .out_cb=pio2_out });
    z80ctc_init(&ctc);

    /* setup keyboard matrix, keep keys pressed for N frames to give
       the scan-out routine enough time
    */
    kbd_init(&kbd, &(keyboard_matrix_desc){ .sticky_count=7 });
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

    /* GLFW keyboard callbacks */
    glfwSetKeyCallback(w, on_key);
    glfwSetCharCallback(w, on_char);

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
    cpu.PC = 0xF000;

    /* emulate and draw frames */
    uint32_t overrun_ticks = 0;
    uint64_t last_time_stamp = stm_now();
    while (!glfwWindowShouldClose(w)) {
        /* number of 2.4576MHz ticks in host frame */
        double frame_time = stm_sec(stm_laptime(&last_time_stamp));
        uint32_t ticks_to_run = (uint32_t) ((KC87_FREQ * frame_time) - overrun_ticks);
        uint32_t ticks_executed = z80_exec(&cpu, ticks_to_run);
        assert(ticks_executed >= ticks_to_run);
        overrun_ticks = ticks_executed - ticks_to_run;
        kbd_update(&kbd);
        gfx_draw(w);
    }
    /* shutdown */
    gfx_shutdown();
    return 0;
}

/* the CPU tick callback performs memory and I/O reads/writes */
uint64_t tick(int num_ticks, uint64_t pins) {

    /* tick the CTC channels, the CTC channel 2 output signal ZCTO2 is connected
       to CTC channel 3 input signal CLKTRG3 to form a timer cascade
       which drives the system clock, store the state of ZCTO2 for the
       next tick
    */
    pins |= ctc_clktrg3_state;
    for (int i = 0; i < num_ticks; i++) {
        if (pins & Z80CTC_ZCTO2) { pins |= Z80CTC_CLKTRG3; }
        else                     { pins &= ~Z80CTC_CLKTRG3; }
        pins = z80ctc_tick(&ctc, pins);

    }
    ctc_clktrg3_state = (pins & Z80CTC_ZCTO2) ? Z80CTC_CLKTRG3:0;

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
        const uint16_t addr = Z80_ADDR(pins);
        if (pins & Z80_RD) {
            /* read memory byte */
            Z80_SET_DATA(pins, mem[addr]);
        }
        else if (pins & Z80_WR) {
            /* write memory byte, don't overwrite ROM */
            if ((addr < 0xC000) || ((addr >= 0xE800) && (addr < 0xF000))) {
                mem[addr] = Z80_DATA(pins);
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
    for (int i = 0; i < num_ticks; i++) {
        pins |= Z80_IEIO;   /* enable the interrupt enable in for highest priority device */
        pins = z80pio_int(&pio1, pins);
        pins = z80pio_int(&pio2, pins);
        pins = z80ctc_int(&ctc, pins);
    }

    return (pins & Z80_PIN_MASK);
}

uint8_t pio1_in(int port_id) {
    return 0x00;
}

void pio1_out(int port_id, uint8_t data) {
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
uint8_t pio2_in(int port_id) {
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

void pio2_out(int port_id, uint8_t data) {
    if (Z80PIO_PORT_A == port_id) {
        kbd_set_active_columns(&kbd, ~data);
    }
    else {
        kbd_set_active_lines(&kbd, ~data);
    }
}

/* decode the KC87 40x24 framebuffer to a linear 320x192 RGBA8 buffer */
void decode_vidmem() {
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
        /* keyboard matrix lines are directly connected to the PIO2's Port B */
        z80pio_write_port(&pio2, Z80PIO_PORT_B, ~kbd_scan_lines(&kbd));
    }
}

/* GLFW 'raw key' callback */
void on_key(GLFWwindow* w, int glfw_key, int scancode, int action, int mods) {
    int key = 0;
    switch (glfw_key) {
        case GLFW_KEY_ENTER:    key = 0x0D; break;
        case GLFW_KEY_RIGHT:    key = 0x09; break;
        case GLFW_KEY_LEFT:     key = 0x08; break;
        case GLFW_KEY_DOWN:     key = 0x0A; break;
        case GLFW_KEY_UP:       key = 0x0B; break;
        case GLFW_KEY_ESCAPE:   key = (mods & GLFW_MOD_SHIFT)? 0x1B: 0x03; break;
        case GLFW_KEY_INSERT:   key = 0x1A; break;
        case GLFW_KEY_HOME:     key = 0x19; break;
    }
    if (key) {
        if ((GLFW_PRESS == action) || (GLFW_REPEAT == action)) {
            kbd_key_down(&kbd, key);
        }
        else if (GLFW_RELEASE == action) {
            kbd_key_up(&kbd, key);
        }
        /* keyboard matrix lines are directly connected to the PIO2's Port B */
        z80pio_write_port(&pio2, Z80PIO_PORT_B, ~kbd_scan_lines(&kbd));
    }
}

/* setup GLFW, flextGL, sokol_gfx, sokol_time and create gfx resources */
GLFWwindow* gfx_init() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* w = glfwCreateWindow(640, 512, "KC87 Example (type 'BASIC')", 0, 0);
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
        .vertex_layouts[0] = {
            .stride = 8,
            .attrs[0] = { .name="pos", .offset=0, .format=SG_VERTEXFORMAT_FLOAT2 }
        },
        .shader = fsq_shd,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP
    });
    /* KC87 framebuffer is 40x24 characters at 8x8 pixels */
    draw_state.fs_images[0] = sg_make_image(&(sg_image_desc){
        .width = 320,
        .height = 192,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = SG_USAGE_STREAM,
    });
    return w;
}

/* decode emulator framebuffer into texture, and draw fullscreen rect */
void gfx_draw(GLFWwindow* w) {
    decode_vidmem();
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

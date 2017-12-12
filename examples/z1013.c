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
#include "chips/keyboard_matrix.h"
#include "roms/z1013-roms.h"
#include <ctype.h> /* isupper, islower, toupper, tolower */

/* the Z1013 hardware */
z80 cpu;
z80pio pio;
keyboard_matrix kbd;
uint8_t kbd_request_column = 0;
bool kbd_request_line_hilo = false;
uint8_t mem[1<<16];

/* emulator callbacks */
uint64_t tick(int num, uint64_t pins);
uint8_t pio_in(int port_id);
void pio_out(int port_id, uint8_t data);

/* GLFW keyboard input callbacks */
void on_key(GLFWwindow* w, int key, int scancode, int action, int mods);
void on_char(GLFWwindow* w, unsigned int codepoint);

/* rendering functions and resources */
GLFWwindow* gfx_init();
void gfx_draw(GLFWwindow* w);
void gfx_shutdown();
sg_draw_state draw_state;
uint32_t rgba8_buffer[256*256];

int main() {

    /* setup rendering via GLFW and sokol_gfx */
    GLFWwindow* w = gfx_init();

    /* initialize the Z80 CPU and PIO */
    z80_init(&cpu, &(z80_desc){ .tick_cb = tick });
    z80pio_init(&pio, &(z80pio_desc){ .in_cb = pio_in, .out_cb = pio_out });

    /* setup the 8x8 keyboard matrix (see http://www.z1013.de/images/21.gif) */
    kbd_init(&kbd, &(keyboard_matrix_desc){
        /* keep keys pressed for at least 2 frames to give the
           Z1013 enough time to scan the keyboard
        */
        .sticky_count = 2,
    });
    /* shift key is column 7, line 6 */
    const int shift = 0, shift_mask = (1<<shift);
    kbd_register_modifier(&kbd, shift, 7, 6);
    /* ctrl key is column 6, line 5 */
    const int ctrl = 1, ctrl_mask = (1<<ctrl);
    kbd_register_modifier(&kbd, ctrl, 6, 5);
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
                    kbd_register_key(&kbd, c, col, line, layer?shift_mask:0);
                }
            }
        }
    }
    /* special keys */
    kbd_register_key(&kbd, ' ',  6, 4, 0);  /* space */
    kbd_register_key(&kbd, 0x08, 6, 2, 0);  /* cursor left */
    kbd_register_key(&kbd, 0x09, 6, 3, 0);  /* cursor right */
    kbd_register_key(&kbd, 0x0A, 6, 7, 0);  /* cursor down */
    kbd_register_key(&kbd, 0x0B, 6, 6, 0);  /* cursor up */
    kbd_register_key(&kbd, 0x0D, 6, 1, 0);  /* enter */
    kbd_register_key(&kbd, 0x03, 1, 3, ctrl_mask); /* map Esc to Ctrl+C (STOP/BREAK) */

    /* GLFW keyboard callbacks */
    glfwSetKeyCallback(w, on_key);
    glfwSetCharCallback(w, on_char);

    /* 2 KByte system rom starting at 0xF000 */
    assert(sizeof(dump_z1013_mon_a2) == 2048);
    memcpy(&mem[0xF000], dump_z1013_mon_a2, sizeof(dump_z1013_mon_a2));

    /* copy BASIC interpreter to 0x0100, skip first 0x20 bytes .z80 file format header */
    assert(0x0100 + sizeof(dump_kc_basic) < 0xF000);
    memcpy(&mem[0x0100], dump_kc_basic+0x20, sizeof(dump_kc_basic)-0x20);

    /* execution starts at 0xF000 */
    cpu.PC = 0xF000;

    /* emulate and draw frames */
    uint32_t overrun_ticks = 0;
    uint64_t last_time_stamp = stm_now();
    while (!glfwWindowShouldClose(w)) {
        /* number of 2MHz ticks in host frame */
        double frame_time = stm_sec(stm_laptime(&last_time_stamp));
        uint32_t ticks_to_run = (uint32_t) ((2000000 * frame_time) - overrun_ticks);
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

/* the CPU tick function needs to perform memory and I/O reads/writes */
uint64_t tick(int num_ticks, uint64_t pins) {
    if (pins & Z80_MREQ) {
        /* a memory request */
        const uint16_t addr = Z80_ADDR(pins);
        if (pins & Z80_RD) {
            /* read memory byte */
            Z80_SET_DATA(pins, mem[addr]);
        }
        else if (pins & Z80_WR) {
            /* write memory byte, don't overwrite ROM */
            if (addr < 0xF000) {
                mem[addr] = Z80_DATA(pins);
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
            pins = z80pio_iorq(&pio, pio_pins) & Z80_PIN_MASK;
        }
        else if ((pins & (Z80_A3|Z80_WR)) == (Z80_A3|Z80_WR)) {
            /* port 8 is connected to a hardware latch to store the
               requested keyboard column for the next keyboard scan
            */
            kbd_request_column = Z80_DATA(pins);
        }
    }
    /* there are no interrupts happening in a vanilla Z1013,
       so don't trigger the interrupt daisy chain
    */
    return pins;
}

/* PIO input callback, scan the upper or lower 4 lines of the keyboard matrix */
uint8_t pio_in(int port_id) {
    uint8_t data = 0;
    if (Z80PIO_PORT_A == port_id) {
        /* PIO port A is reserved for user devices */
        data = 0xFF;
    }
    else {
        /* port B is for cassette input (bit 7), and lower 4 bits for kbd matrix lines */
        uint16_t column_mask = (1<<kbd_request_column);
        uint16_t line_mask = kbd_test_lines(&kbd, column_mask);
        if (kbd_request_line_hilo) {
            line_mask >>= 4;
        }
        data = 0xF & ~(line_mask & 0xF);
    }
    return data;
}

/* the PIO output callback selects the upper or lower 4 lines for the next keyboard scan */
void pio_out(int port_id, uint8_t data) {
    if (Z80PIO_PORT_B == port_id) {
        /* bit 4 for 8x8 keyboard selects upper or lower 4 kbd matrix line bits */
        kbd_request_line_hilo = 0 != (data & (1<<4));
        /* bit 7 is cassette output, not emulated */
    }
}

/* decode the Z1013 32x32 ASCII framebuffer to a linear 256x256 RGBA8 buffer */
void decode_vidmem() {
    uint32_t* dst = rgba8_buffer;
    const uint8_t* src = &mem[0xEC00];   /* the 32x32 framebuffer starts at EC00 */
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
        case GLFW_KEY_ENTER:    key = 0x0D; break;
        case GLFW_KEY_RIGHT:    key = 0x09; break;
        case GLFW_KEY_LEFT:     key = 0x08; break;
        case GLFW_KEY_DOWN:     key = 0x0A; break;
        case GLFW_KEY_UP:       key = 0x0B; break;
        case GLFW_KEY_ESCAPE:   key = 0x03; break;
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
    GLFWwindow* w = glfwCreateWindow(512, 512, "Z1013 Example ('j 300' to start BASIC)", 0, 0);
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
    /* z1013 framebuffer is 32x32 characters at 8x8 pixels */
    draw_state.fs_images[0] = sg_make_image(&(sg_image_desc){
        .width = 256,
        .height = 256,
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

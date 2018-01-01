/*
    atom.c

    The Acorn Atom was a very simple 6502-based home computer
    consisting of just a MOS 6502 CPU, Motorola MC6847 video
    display generator, and Intel i8255 I/O chip.

    The audio beeper is not emulated.
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
int period_2_4hz = 0;
uint8_t ram[1<<16];

/* emulator callbacks */
uint64_t cpu_tick(uint64_t pins);
uint64_t vdg_fetch(uint64_t pins);
uint8_t ppi_in(int port_id);
uint64_t ppi_out(int port_id, uint64_t pins, uint8_t data);

/* GLFW keyboard input callbacks */
void on_key(GLFWwindow* w, int key, int scancode, int action, int mods);
void on_char(GLFWwindow* w, unsigned int codepoint);

/* rendering functions and resources */
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
    for (int i = 0; i < (int)sizeof(ram); i++) {
        uint32_t r = xorshift32();
        ram[i++] = r>>24;
        ram[i++] = r>>16;
        ram[i++] = r>>8;
        ram[i++] = r;
    }
    mem_init(&mem);
    /* 32 KByte RAM + 8 KByte vidmem */
    mem_map_ram(&mem, 0, 0x0000, 0xA000, ram);
    /* hole in 0xA000 to 0xAFFF for utility roms */
    /* 0xB000 to 0xBFFF is memory-mapped IO area */
    /* 0xC000 to 0xFFFF are operating system roms */
    mem_map_rom(&mem, 0, 0xC000, 0x1000, dump_abasic);
    mem_map_rom(&mem, 0, 0xF000, 0x1000, dump_afloat);
    mem_map_rom(&mem, 0, 0xE000, 0x1000, dump_dosrom);
    mem_map_rom(&mem, 0, 0xF000, 0x1000, dump_abasic+0x1000);

    /*  setup the keyboard matrix
        the Atom has a 10x7 keyboard matrix, where the
        entire line 6 is for the Ctrl key, and the entire
        line 7 is the shift key
    */
    kbd_init(&kbd, 2);
    /* alpha-numeric keys */
    const char* keymap = 
        /* no shift */
        "     ^]\\[ "   /* line 0 */
        "3210      "    /* line 1 */
        "-,;:987654"    /* line 2 */
        "GFEDCBA@/."    /* line 3 */
        "QPONMLKJIH"    /* line 4 */
        " ZYXWVUTSR"    /* line 5 */
        /* shift */
        "          "    /* line 0 */
        "#\"!       "   /* line 1 */
        "=<+*)('&%$"    /* line 2 */
        "gfedcba ?>"    /* line 3 */
        "qponmlkjih"    /* line 4 */
        " zyxwvutsr";   /* line 5 */
    for (int shift=0; shift < 2; shift++) {
        for (int col = 0; col < 10; col++) {
            for (int line = 0; line < 6; line++) {
                int c = keymap[shift*60 + line*10 + col];
                if (c != 0x20) {
                    kbd_register_key(&kbd, c, col, line, shift?(1<<0):0);
                }
            }
        }
    }
    /* FIXME: special keys */

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

    /* GLFW keyboard callbacks */
    glfwSetKeyCallback(w, on_key);
    glfwSetCharCallback(w, on_char);

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
    // FIXME!
    return pins;
}

/* video memory fetch callback */
uint64_t vdg_fetch(uint64_t pins) {
    // FIXME!
    return pins;
}

/* i8255 PPI input callback */
uint8_t ppi_in(int port_id) {
    // FIXME!
    return 0xFF;
}

/* i8255 PPI output callback */
uint64_t ppi_out(int port_id, uint64_t pins, uint8_t data) {
    // FIXME!
    return pins;
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
        .vertex_layouts[0] = {
            .stride = 8,
            .attrs[0] = { .name="pos", .offset=0, .format=SG_VERTEXFORMAT_FLOAT2 }
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

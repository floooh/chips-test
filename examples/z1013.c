/*
    z1013.c

    The Z1013 was a very simple East German home computer, basically
    just a Z80 CPU, PIO, some memory and a keyboard matrix. It's easy
    to emulate because the system didn't use any interrupts, and only
    simple PIO IN/OUT is required to control the keyboard.

    It had a 32x32 mono-chrome ASCII framebuffer starting at EC00,
    and a 2 KByte operating system ROM starting at F000.

    FIXME: keyboard input

    No cassette-tape / beeper sound emulated!
*/
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "flextgl/flextGL.h"
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol_gfx.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "z1013-roms.h"

/* rendering functions and resources */
GLFWwindow* init_gfx();
void draw_frame(GLFWwindow* w);
void shutdown_gfx();
sg_draw_state draw_state;
uint32_t rgba8_buffer[256*256];

/* emulator callbacks */
uint64_t tick(uint64_t pins);
uint8_t pio_in(int port_id);
void pio_out(int port_id, uint8_t data);

/* emulator chips and 64 KBytes of memory */
z80 cpu;
z80pio pio;
uint8_t mem[1<<16];

int main() {
    /* setup rendering */
    GLFWwindow* w = init_gfx();

    /* initialize the Z80 CPU and PIO */
    z80_init(&cpu, &(z80_desc){
        .tick_cb = tick
    });
    z80pio_init(&pio, &(z80pio_desc){
        .in_cb = pio_in,
        .out_cb = pio_out
    });
    /* 2 KByte system rom starting at 0xF000 */
    assert(sizeof(dump_z1013_mon_a2) == 2048);
    memcpy(&mem[0xF000], dump_z1013_mon_a2, sizeof(dump_z1013_mon_a2));

    /* execution starts at 0xF000 */
    cpu.PC = 0xF000;

    /* emulate and draw frames */
    uint32_t overrun_ticks = 0;
    while (!glfwWindowShouldClose(w)) {
        /* number of 2MHz ticks in a 60Hz frame */
        uint32_t ticks_to_run = (2000000 / 60) + overrun_ticks;
        uint32_t ticks_executed = z80_run(&cpu, ticks_to_run);
        assert(ticks_executed >= ticks_to_run);
        overrun_ticks = ticks_executed - ticks_to_run;
        draw_frame(w);
    }
    /* shutdown */
    shutdown_gfx();
    return 0;
}

uint8_t pio_in(int port_id) {
    // FIXME
    return 0x00;
}

void pio_out(int port_id, uint8_t data) {
    // FIXME
}

/* the CPU tick function needs to perform memory and I/O reads/writes */
uint64_t tick(uint64_t pins) {
    if (pins & Z80_MREQ) {
        /* a memory request */
        const uint16_t addr = Z80_ADDR(pins);
        if (pins & Z80_RD) {
            /* read memory byte */
            Z80_SET_DATA(pins, mem[addr]);
        }
        else if (pins & Z80_WR) {
            if (addr < 0xF000) {
                /* write memory byte, don't overwrite ROM */
                uint8_t data = Z80_DATA(pins);
                mem[addr] = data;
            }
        }
    }
    else if (pins & Z80_IORQ) {
        /* an I/O request */
        if (pins & Z80_RD) {
            // FIXME!
            Z80_SET_DATA(pins, 0xFF);
        }
        else if (pins & Z80_WR) {
            // FIXME!
        }
    }
    return pins;
}

GLFWwindow* init_gfx() {
    /* create window and GL context via GLFW */
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* w = glfwCreateWindow(640, 640, "Z1013 Example", 0, 0);
    glfwMakeContextCurrent(w);
    glfwSwapInterval(1);
    flextInit(w);

    /* setup sokol_gfx and sokol_time */
    sg_setup(&(sg_desc){});

    /* rendering resources for textured fullscreen rectangle */
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
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });
    return w;
}

/* decode the Z1013 32x32 ASCII framebuffer to a linear 256x256 RGBA8 buffer */
void decode_video() {
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

/* decode emulator framebuffer into texture, and draw fullscreen rect */
void draw_frame(GLFWwindow* w) {
    decode_video();
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

void shutdown_gfx() {
    sg_shutdown();
    glfwTerminate();
}

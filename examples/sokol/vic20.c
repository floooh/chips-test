/*
    vic20.c
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/m6522.h"
#include "chips/m6561.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/vic20.h"
#include "vic20-roms.h"

/* imports from cpc-ui.cc */
#ifdef CHIPS_USE_UI
#include "ui.h"
void vic20ui_init(vic20_t* vic20);
void vic20ui_discard(void);
void vic20ui_draw(void);
void vic20ui_exec(vic20_t* vic20, uint32_t frame_time_us);
static const int ui_extra_height = 16;
#else
static const int ui_extra_height = 0;
#endif

vic20_t vic20;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){
        .argc=argc,
        .argv=argv,
        .buf_size = 512 * 1024,
    });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 3 * vic20_std_display_width(),
        .height = 2 * vic20_std_display_height() + ui_extra_height,
        .window_title = "VIC-20",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* get c64_desc_t struct based on joystick type */
vic20_desc_t vic20_desc(vic20_joystick_type_t joy_type, vic20_memory_config_t mem_config) {
    return (vic20_desc_t) {
        .joystick_type = joy_type,
        .mem_config = mem_config,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .audio_volume = 0.3f,
        .rom_char = dump_vic20_characters_901460_03_bin,
        .rom_char_size = sizeof(dump_vic20_characters_901460_03_bin),
        .rom_basic = dump_vic20_basic_901486_01_bin,
        .rom_basic_size = sizeof(dump_vic20_basic_901486_01_bin),
        .rom_kernal = dump_vic20_kernal_901486_07_bin,
        .rom_kernal_size = sizeof(dump_vic20_kernal_901486_07_bin)
    };
}

/* one-time application init */
void app_init(void) {
    gfx_init(&(gfx_desc_t){
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .top_offset = ui_extra_height,
        .aspect_x = 3.0f,
        .aspect_y = 2.0f
    });
    keybuf_init(5);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
    bool delay_input = false;
    if (sargs_exists("file")) {
        delay_input = true;
        if (!fs_load_file(sargs_value("file"))) {
            gfx_flash_error();
        }
    }
    if (sargs_exists("prg")) {
        if (!fs_load_base64("url.prg", sargs_value("prg"))) {
            gfx_flash_error();
        }
    }
    vic20_joystick_type_t joy_type = VIC20_JOYSTICKTYPE_NONE;
    if (sargs_exists("joystick")) {
        joy_type = VIC20_JOYSTICKTYPE_DIGITAL;
    }
    vic20_memory_config_t mem_config = VIC20_MEMCONFIG_STANDARD;
    if (sargs_exists("exp")) {
        if (sargs_equals("exp", "ram8k")) {
            mem_config = VIC20_MEMCONFIG_8K;
        }
        else if (sargs_equals("exp", "ram16k")) {
            mem_config = VIC20_MEMCONFIG_16K;
        }
        else if (sargs_equals("exp", "ram24k")) {
            mem_config = VIC20_MEMCONFIG_24K;
        }
        else if (sargs_equals("exp", "ram32k")) {
            mem_config = VIC20_MEMCONFIG_32K;
        }
    }
    vic20_desc_t desc = vic20_desc(joy_type, mem_config);
    vic20_init(&vic20, &desc);
    #ifdef CHIPS_USE_UI
    vic20ui_init(&vic20);
    #endif
    if (!delay_input) {
        if (sargs_exists("input")) {
            keybuf_put(sargs_value("input"));
        }
    }
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame(void) {
    #ifdef CHIPS_USE_UI
        vic20ui_exec(&vic20, clock_frame_time());
    #else
        vic20_exec(&vic20, clock_frame_time());
    #endif
    gfx_draw(vic20_display_width(&vic20), vic20_display_height(&vic20));
    const uint32_t load_delay_frames = 180;
    if (fs_ptr() && clock_frame_count() > load_delay_frames) {
        bool load_success = false;
        if (fs_ext("txt") || fs_ext("bas")) {
            load_success = true;
            keybuf_put((const char*)fs_ptr());
        }
        else if (fs_ext("bin") || fs_ext("prg") || fs_ext("")) {
            load_success = vic20_quickload(&vic20, fs_ptr(), fs_size());
        }
        if (load_success) {
            if (clock_frame_count() > (load_delay_frames + 10)) {
                gfx_flash_success();
            }
        }
        else {
            gfx_flash_error();
        }
        fs_free();
    }
    uint8_t key_code;
    if (0 != (key_code = keybuf_get())) {
        /* FIXME: this is ugly */
        vic20_joystick_type_t joy_type = vic20.joystick_type;
        vic20.joystick_type = VIC20_JOYSTICKTYPE_NONE;
        vic20_key_down(&vic20, key_code);
        vic20_key_up(&vic20, key_code);
        vic20.joystick_type = joy_type;
    }
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        /* input was handled by UI */
        return;
    }
    #endif
    const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c > 0x20) && (c < 0x7F)) {
                /* need to invert case (unshifted is upper caps, shifted is lower caps */
                if (isupper(c)) {
                    c = tolower(c);
                }
                else if (islower(c)) {
                    c = toupper(c);
                }
                vic20_key_down(&vic20, c);
                vic20_key_up(&vic20, c);
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
                    vic20_key_down(&vic20, c);
                }
                else {
                    vic20_key_up(&vic20, c);
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
    #ifdef CHIPS_USE_UI
    vic20ui_discard();
    #endif
    vic20_discard(&vic20);
    saudio_shutdown();
    gfx_shutdown();
}

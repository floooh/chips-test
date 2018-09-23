/*
    cpc.c

    Amstrad CPC 464/6128 and KC Compact. No disc emulation.
*/
#include "common.h"
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
#include "cpc-roms.h"

cpc_t cpc;

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
        .width = CPC_DISPLAY_WIDTH,
        .height = 2 * CPC_DISPLAY_HEIGHT,
        .window_title = "CPC 6128",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* one-time application init */
void app_init(void) {
    gfx_init(CPC_DISPLAY_WIDTH, CPC_DISPLAY_HEIGHT, 1, 2);
    keybuf_init(10);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
    if (args_has("file")) {
        fs_load_file(args_string("file"));
    }
    if (args_has("tape")) {
        fs_load_file(args_string("tape"));
    }
    cpc_type_t type = CPC_TYPE_6128;
    if (args_has("type")) {
        if (args_string_compare("type", "cpc464")) {
            type = CPC_TYPE_464;
        }
        else if (args_string_compare("type", "kccompact")) {
            type = CPC_TYPE_KCCOMPACT;
        }
    }
    cpc_joystick_type_t joy_type = CPC_JOYSTICK_NONE;
    if (args_has("joystick")) {
        joy_type = CPC_JOYSTICK_DIGITAL;
    }
    cpc_init(&cpc, &(cpc_desc_t){
        .type = type,
        .joystick_type = joy_type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .audio_cb = push_audio,
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
    /* load a data file? FIXME: this needs proper file type detection in case
       a file has been dragged'n'dropped
    */
    if (fs_ptr() && clock_frame_count() > 120) {
        if (args_has("tape")) {
            /* load the file data via the tape-emulation (TAP format) */
            if (cpc_insert_tape(&cpc, fs_ptr(), fs_size())) {
                /* issue the right commands to start loading from tape */
                keybuf_put("|tape\nrun\"\n\n");
            }
        }
        else {
            /* load the file data as a quickload-snapshot (SNA or BIN format)*/
            cpc_quickload(&cpc, fs_ptr(), fs_size());
        }
        fs_free();
    }
    uint8_t key_code;
    if (0 != (key_code = keybuf_get())) {
        cpc_key_down(&cpc, key_code);
        cpc_key_up(&cpc, key_code);
    }
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c > 0x20) && (c < 0x7F)) {
                cpc_key_down(&cpc, c);
                cpc_key_up(&cpc, c);
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
                    cpc_key_down(&cpc, c);
                }
                else {
                    cpc_key_up(&cpc, c);
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

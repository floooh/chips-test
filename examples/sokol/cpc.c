/*
    cpc.c

    Amstrad CPC 464/6128 and KC Compact. No disc emulation.
*/
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "common.h"
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/i8255.h"
#include "chips/mc6845.h"
#include "chips/am40010.h"
#include "chips/upd765.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/fdd.h"
#include "chips/fdd_cpc.h"
#include "systems/cpc.h"
#include "cpc-roms.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_Z80
    #include "ui.h"
    #include "ui/ui_settings.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_z80.h"
    #include "ui/ui_ay38910.h"
    #include "ui/ui_mc6845.h"
    #include "ui/ui_am40010.h"
    #include "ui/ui_i8255.h"
    #include "ui/ui_upd765.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_display.h"
    #include "ui/ui_kbd.h"
    #include "ui/ui_fdd.h"
    #include "ui/ui_snapshot.h"
    #include "ui/ui_cpc.h"
#endif
#include <stdlib.h> // calloc,free

typedef struct {
    uint32_t version;
    cpc_t cpc;
} cpc_snapshot_t;

static struct {
    cpc_t cpc;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    #if defined(CHIPS_USE_UI)
        ui_cpc_t ui;
        struct {
            uint32_t entry_addr;
            uint32_t exit_addr;
            bool entered;
        } dbg;
        cpc_snapshot_t snapshots[UI_SNAPSHOT_MAX_SLOTS];
    #endif
} state;

#ifdef CHIPS_USE_UI
#define BORDER_TOP (24)
static void ui_draw_cb(const ui_draw_info_t* draw_info);
static void ui_save_settings_cb(ui_settings_t* settings);
static void ui_boot_cb(cpc_t* sys, cpc_type_t type);
static void ui_save_snapshot(size_t slot_index);
static bool ui_load_snapshot(size_t slot_index);
static void ui_load_snapshots_from_storage(void);
static void web_boot(void);
static void web_reset(void);
static bool web_ready(void);
static bool web_load(chips_range_t data);
static void web_input(const char*);
static bool web_load_file(const char *file);
static bool web_unload_file();
static void web_save_snapshot(size_t slot_index);
static bool web_load_snapshot(size_t slot_index);
static void web_dbg_connect(void);
static void web_dbg_disconnect(void);
static void web_dbg_add_breakpoint(uint16_t addr);
static void web_dbg_remove_breakpoint(uint16_t addr);
static void web_dbg_break(void);
static void web_dbg_continue(void);
static void web_dbg_step_next(void);
static void web_dbg_step_into(void);
static void web_dbg_on_stopped(int stop_reason, uint16_t addr);
static void web_dbg_on_continued(void);
static void web_dbg_on_reboot(void);
static void web_dbg_on_reset(void);
static webapi_cpu_state_t web_dbg_cpu_state(void);
static void web_dbg_request_disassemly(uint16_t addr, int offset_lines, int num_lines, webapi_dasm_line_t* result);
static void web_dbg_read_memory(uint16_t addr, int num_bytes, uint8_t* dst_ptr);
#else
#define BORDER_TOP (8)
#endif
#define BORDER_LEFT (8)
#define BORDER_RIGHT (8)
#define BORDER_BOTTOM (32)
#define LOAD_DELAY_FRAMES (120)

// audio-streaming callback
static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

// get cpc_desc_t struct based on model and joystick type
cpc_desc_t cpc_desc(cpc_type_t type, cpc_joystick_type_t joy_type) {
    return (cpc_desc_t) {
        .type = type,
        .joystick_type = joy_type,
        .audio = {
            .callback = { .func=push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .roms = {
            .cpc464 = {
                .os = { .ptr=dump_cpc464_os_bin, .size=sizeof(dump_cpc464_os_bin) },
                .basic = { .ptr=dump_cpc464_basic_bin, .size=sizeof(dump_cpc464_basic_bin) },
            },
            .cpc6128 = {
                .os = { .ptr=dump_cpc6128_os_bin, .size=sizeof(dump_cpc6128_os_bin) },
                .basic = { .ptr=dump_cpc6128_basic_bin, .size= sizeof(dump_cpc6128_basic_bin) },
                .amsdos = { .ptr=dump_cpc6128_amsdos_bin, .size=sizeof(dump_cpc6128_amsdos_bin) }
            },
            .kcc = {
                .os = { .ptr=dump_kcc_os_bin, .size=sizeof(dump_kcc_os_bin) },
                .basic = { .ptr=dump_kcc_bas_bin, .size=sizeof(dump_kcc_bas_bin) }
            },
        },
        #if defined(CHIPS_USE_UI)
        .debug = ui_cpc_get_debug(&state.ui),
        #endif
    };
}

void app_init(void) {
    saudio_setup(&(saudio_desc){
        .logger.func = slog_func,
    });
    cpc_type_t type = CPC_TYPE_6128;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "cpc464")) {
            type = CPC_TYPE_464;
        } else if (sargs_equals("type", "kccompact")) {
            type = CPC_TYPE_KCCOMPACT;
        }
    }
    cpc_joystick_type_t joy_type = CPC_JOYSTICK_NONE;
    if (sargs_exists("joystick")) {
        joy_type = CPC_JOYSTICK_DIGITAL;
    }
    cpc_desc_t desc = cpc_desc(type, joy_type);
    cpc_init(&state.cpc, &desc);
    gfx_init(&(gfx_desc_t){
        .disable_speaker_icon = sargs_exists("disable-speaker-icon"),
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .border = {
            .left = BORDER_LEFT,
            .right = BORDER_RIGHT,
            .top = BORDER_TOP,
            .bottom = BORDER_BOTTOM,
        },
        .display_info = cpc_display_info(&state.cpc),
        .pixel_aspect = {
            .width = 1,
            .height = 2,
        }
    });
    keybuf_init(&(keybuf_desc_t) { .key_delay_frames=7 });
    clock_init();
    prof_init();
    fs_init();
    #ifdef CHIPS_USE_UI
        ui_init(&(ui_desc_t){
            .draw_cb = ui_draw_cb,
            .save_settings_cb = ui_save_settings_cb,
            .imgui_ini_key = "floooh.chips.cpc",
        });
        ui_cpc_init(&state.ui, &(ui_cpc_desc_t){
            .cpc = &state.cpc,
            .boot_cb = ui_boot_cb,
            .dbg_texture = {
                .create_cb = ui_create_texture,
                .update_cb = ui_update_texture,
                .destroy_cb = ui_destroy_texture,
            },
            .dbg_debug = {
                .reboot_cb = web_dbg_on_reboot,
                .reset_cb = web_dbg_on_reset,
                .stopped_cb = web_dbg_on_stopped,
                .continued_cb = web_dbg_on_continued,
            },
            .snapshot = {
                .load_cb = ui_load_snapshot,
                .save_cb = ui_save_snapshot,
                .empty_slot_screenshot = {
                    .texture = ui_shared_empty_snapshot_texture(),
                }
            },
            .dbg_keys = {
                .cont = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F5), .name = "F5" },
                .stop = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F5), .name = "F5" },
                .step_over = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F6), .name = "F6" },
                .step_into = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F7), .name = "F7" },
                .step_tick = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F8), .name = "F8" },
                .toggle_breakpoint = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F9), .name = "F9" }
            }
        });
        ui_cpc_load_settings(&state.ui, ui_settings());
        ui_load_snapshots_from_storage();
        // important: initialize webapi after ui
        webapi_init(&(webapi_desc_t){
            .funcs = {
                .boot = web_boot,
                .reset = web_reset,
                .ready = web_ready,
                .load = web_load,
                .input = web_input,
                .load_file = web_load_file,
                .unload_file = web_unload_file,
                .load_snapshot = web_load_snapshot,
                .save_snapshot = web_save_snapshot,
                .dbg_connect = web_dbg_connect,
                .dbg_disconnect = web_dbg_disconnect,
                .dbg_add_breakpoint = web_dbg_add_breakpoint,
                .dbg_remove_breakpoint = web_dbg_remove_breakpoint,
                .dbg_break = web_dbg_break,
                .dbg_continue = web_dbg_continue,
                .dbg_step_next = web_dbg_step_next,
                .dbg_step_into = web_dbg_step_into,
                .dbg_cpu_state = web_dbg_cpu_state,
                .dbg_request_disassembly = web_dbg_request_disassemly,
                .dbg_read_memory = web_dbg_read_memory,
            }
        });
    #endif

    bool delay_input = false;
    if (sargs_exists("file")) {
        delay_input = true;
        fs_load_file_async(FS_CHANNEL_IMAGES, sargs_value("file"));
    }
    if (!delay_input) {
        if (sargs_exists("input")) {
            keybuf_put(sargs_value("input"));
        }
    }
}

static void handle_file_loading(void);
static void send_keybuf_input(void);
static void draw_status_bar(void);

void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t emu_start_time = stm_now();
    state.ticks = cpc_exec(&state.cpc, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(cpc_display_info(&state.cpc));
    handle_file_loading();
    send_keybuf_input();
}

void app_input(const sapp_event* event) {
    // accept dropped files also when ImGui grabs input
    if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
        fs_load_dropped_file_async(FS_CHANNEL_IMAGES);
    }
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        // input was handled by UI
        return;
    }
    #endif
    const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
    switch (event->type) {
        case SAPP_EVENTTYPE_CHAR:
            {
                int c = (int) event->char_code;
                if ((c > 0x20) && (c < 0x7F)) {
                    cpc_key_down(&state.cpc, c);
                    cpc_key_up(&state.cpc, c);
                }
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            {
                int c = 0;
                int shift_c = 0;
                switch (event->key_code) {
                    case SAPP_KEYCODE_SPACE:        c = 0x20; break;
                    case SAPP_KEYCODE_LEFT:         c = 0x08; break;
                    case SAPP_KEYCODE_RIGHT:        c = 0x09; break;
                    case SAPP_KEYCODE_DOWN:         c = 0x0A; break;
                    case SAPP_KEYCODE_UP:           c = 0x0B; break;
                    case SAPP_KEYCODE_ENTER:        c = 0x0D; break;
                    case SAPP_KEYCODE_LEFT_SHIFT:   c = 0x02; break;
                    case SAPP_KEYCODE_BACKSPACE:    c = 0x01; shift_c = 0x0C; break; // 0x0C: clear screen
                    case SAPP_KEYCODE_ESCAPE:       c = 0x03; shift_c = 0x13; break; // 0x13: break
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
                        if (shift_c == 0) {
                            shift_c = c;
                        }
                        cpc_key_down(&state.cpc, shift ? shift_c : c);
                    } else {
                        // see: https://github.com/floooh/chips-test/issues/20
                        cpc_key_up(&state.cpc, c);
                        if (shift_c) {
                            cpc_key_up(&state.cpc, shift_c);
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
}

void app_cleanup(void) {
    cpc_discard(&state.cpc);
    #ifdef CHIPS_USE_UI
        ui_cpc_discard(&state.ui);
        ui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

static void send_keybuf_input(void) {
    uint8_t key_code;
    if (0 != (key_code = keybuf_get(state.frame_time_us))) {
        cpc_key_down(&state.cpc, key_code);
        cpc_key_up(&state.cpc, key_code);
    }
}

static void handle_file_loading(void) {
    fs_dowork();
    const uint32_t load_delay_frames = LOAD_DELAY_FRAMES;
    if (fs_success(FS_CHANNEL_IMAGES) && ((clock_frame_count_60hz() > load_delay_frames) || fs_ext(FS_CHANNEL_IMAGES, "sna"))) {
        bool load_success = false;
        if (fs_ext(FS_CHANNEL_IMAGES, "txt") || fs_ext(FS_CHANNEL_IMAGES, "bas")) {
            load_success = true;
            keybuf_put((const char*)fs_data(FS_CHANNEL_IMAGES).ptr);
        }
        /*
        else if (fs_ext("tap")) {
            load_success = cpc_insert_tape(&state.cpc, fs_ptr(), fs_size());
        }
        */
        else if (fs_ext(FS_CHANNEL_IMAGES, "dsk")) {
            load_success = cpc_insert_disc(&state.cpc, fs_data(FS_CHANNEL_IMAGES));
        } else if (fs_ext(FS_CHANNEL_IMAGES, "sna") || fs_ext(FS_CHANNEL_IMAGES, "bin")) {
            load_success = cpc_quickload(&state.cpc, fs_data(FS_CHANNEL_IMAGES), true);
        }
        if (load_success) {
            if (clock_frame_count_60hz() > (load_delay_frames + 10)) {
                gfx_flash_success();
            }
            if (sargs_exists("input")) {
                keybuf_put(sargs_value("input"));
            }
        } else {
            gfx_flash_error();
        }
        fs_reset(FS_CHANNEL_IMAGES);
    }
}

static void draw_status_bar(void) {
    prof_push(PROF_EMU, (float)state.emu_time_ms);
    prof_stats_t emu_stats = prof_stats(PROF_EMU);

    const uint32_t text_color = 0xFFFFFFFF;
    const uint32_t disc_active = 0xFF00EE00;
    const uint32_t disc_inactive = 0xFF006600;
    const uint32_t motor_active = 0xFF00CCEE;
    const uint32_t motor_inactive = 0xFF004466;
    const uint32_t joy_active = 0xFFFFEE00;
    const uint32_t joy_inactive = 0xFF886600;

    const float w = sapp_widthf();
    const float h = sapp_heightf();
    sdtx_canvas(w, h);
    sdtx_origin(1.0f, (h / 8.0f) - 3.5f);
    sdtx_font(0);

    // joystick state
    sdtx_puts("JOYSTICK: ");
    sdtx_font(1);
    const uint8_t joymask = cpc_joystick_mask(&state.cpc);
    sdtx_color1i((joymask & CPC_JOYSTICK_LEFT) ? joy_active : joy_inactive);
    sdtx_putc(0x88); // arrow left
    sdtx_color1i((joymask & CPC_JOYSTICK_RIGHT) ? joy_active : joy_inactive);
    sdtx_putc(0x89); // arrow right
    sdtx_color1i((joymask & CPC_JOYSTICK_UP) ? joy_active : joy_inactive);
    sdtx_putc(0x8B); // arrow up
    sdtx_color1i((joymask & CPC_JOYSTICK_DOWN) ? joy_active : joy_inactive);
    sdtx_putc(0x8A); // arrow down
    sdtx_color1i((joymask & CPC_JOYSTICK_BTN0) ? joy_active : joy_inactive);
    sdtx_putc(0x87); // btn
    sdtx_font(0);

    // FDD disc inserted LED
    sdtx_color1i(text_color);
    sdtx_puts("  DISC: ");
    sdtx_color1i(cpc_disc_inserted(&state.cpc) ? disc_active : disc_inactive);
    sdtx_putc(0xCF);    // filled circle
    // FDD motor on LED
    sdtx_color1i(text_color);
    sdtx_puts("  MOTOR: ");
    sdtx_color1i(state.cpc.fdd.motor_on ? motor_active : motor_inactive);
    sdtx_putc(0xCF);

    sdtx_color1i(text_color);
    sdtx_printf("  TRACK:%d", state.cpc.fdd.cur_track_index);

    sdtx_font(0);
    sdtx_color1i(text_color);
    sdtx_pos(0.0f, 1.5f);
    sdtx_printf("frame:%.2fms emu:%.2fms (min:%.2fms max:%.2fms) ticks:%d", (float)state.frame_time_us * 0.001f, emu_stats.avg_val, emu_stats.min_val, emu_stats.max_val, state.ticks);
}

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(const ui_draw_info_t* draw_info) {
    ui_cpc_draw(&state.ui, &(ui_cpc_frame_t){
        .display = draw_info->display,
    });
}

static void ui_save_settings_cb(ui_settings_t* settings) {
    ui_cpc_save_settings(&state.ui, settings);
}

static void ui_boot_cb(cpc_t* sys, cpc_type_t type) {
    clock_init();
    cpc_desc_t desc = cpc_desc(type, sys->joystick_type);
    cpc_init(sys, &desc);
}

static void ui_update_snapshot_screenshot(size_t slot) {
    ui_snapshot_screenshot_t screenshot = {
        .texture = ui_create_screenshot_texture(cpc_display_info(&state.snapshots[slot].cpc))
    };
    ui_snapshot_screenshot_t prev_screenshot = ui_snapshot_set_screenshot(&state.ui.snapshot, slot, screenshot);
    if (prev_screenshot.texture) {
        ui_destroy_texture(prev_screenshot.texture);
    }
}

static void ui_save_snapshot(size_t slot) {
    if (slot < UI_SNAPSHOT_MAX_SLOTS) {
        state.snapshots[slot].version = cpc_save_snapshot(&state.cpc, &state.snapshots[slot].cpc);
        ui_update_snapshot_screenshot(slot);
        fs_save_snapshot("cpc", slot, (chips_range_t){ .ptr = &state.snapshots[slot], sizeof(cpc_snapshot_t) });
    }
}

static bool ui_load_snapshot(size_t slot) {
    bool success = false;
    if ((slot < UI_SNAPSHOT_MAX_SLOTS) && (state.ui.snapshot.slots[slot].valid)) {
        success = cpc_load_snapshot(&state.cpc, state.snapshots[slot].version, &state.snapshots[slot].cpc);
    }
    return success;
}

static void ui_fetch_snapshot_callback(const fs_snapshot_response_t* response) {
    assert(response);
    if (response->result != FS_RESULT_SUCCESS) {
        return;
    }
    if (response->data.size != sizeof(cpc_snapshot_t)) {
        return;
    }
    if (((cpc_snapshot_t*)response->data.ptr)->version != CPC_SNAPSHOT_VERSION) {
        return;
    }
    size_t snapshot_slot = response->snapshot_index;
    assert(snapshot_slot < UI_SNAPSHOT_MAX_SLOTS);
    memcpy(&state.snapshots[snapshot_slot], response->data.ptr, response->data.size);
    ui_update_snapshot_screenshot(snapshot_slot);
}

static void ui_load_snapshots_from_storage(void) {
    for (size_t snapshot_slot = 0; snapshot_slot < UI_SNAPSHOT_MAX_SLOTS; snapshot_slot++) {
        fs_load_snapshot_async("cpc", snapshot_slot, ui_fetch_snapshot_callback);
    }
}

// webapi wrappers
static void web_boot(void) {
    clock_init();
    cpc_desc_t desc = cpc_desc(state.cpc.type, state.cpc.joystick_type);
    cpc_init(&state.cpc, &desc);
    ui_dbg_reboot(&state.ui.dbg);
}

static void web_reset(void) {
    cpc_reset(&state.cpc);
    ui_dbg_reset(&state.ui.dbg);
}

static void web_dbg_connect(void) {
    gfx_disable_speaker_icon();
    state.dbg.entry_addr = 0xFFFFFFFF;
    state.dbg.exit_addr = 0xFFFFFFFF;
    state.dbg.entered = false;
    ui_dbg_external_debugger_connected(&state.ui.dbg);
}

static void web_dbg_disconnect(void) {
    state.dbg.entry_addr = 0xFFFFFFFF;
    state.dbg.exit_addr = 0xFFFFFFFF;
    state.dbg.entered = false;
    ui_dbg_external_debugger_disconnected(&state.ui.dbg);
}

static bool web_ready(void) {
    return clock_frame_count_60hz() > LOAD_DELAY_FRAMES;
}

static bool web_load(chips_range_t data) {
    if (data.size <= sizeof(webapi_fileheader_t)) {
        return false;
    }
    const webapi_fileheader_t* hdr = (webapi_fileheader_t*)data.ptr;
    if ((hdr->type[0] != 'B') || (hdr->type[1] != 'I') || (hdr->type[2] != 'N') || (hdr->type[3] != ' ')) {
        return false;
    }
    const bool start = 0 != (hdr->flags & WEBAPI_FILEHEADER_FLAG_START);
    const chips_range_t bin = { .ptr = (void*)&hdr->payload, .size = data.size - sizeof(webapi_fileheader_t) };
    bool loaded = cpc_quickload(&state.cpc, bin, start);
    if (loaded) {
        state.dbg.entry_addr = cpc_quickload_exec_addr(bin);
        state.dbg.exit_addr = cpc_quickload_return_addr(&state.cpc);
        ui_dbg_add_breakpoint(&state.ui.dbg, state.dbg.entry_addr);
        ui_dbg_add_breakpoint(&state.ui.dbg, state.dbg.exit_addr);
        // if debugger is stopped, unstuck it
        if (ui_dbg_stopped(&state.ui.dbg)) {
            ui_dbg_continue(&state.ui.dbg, false);
        }
    }
    return loaded;
}

static bool web_load_file(const char* file) {
    fs_load_file_async(FS_CHANNEL_IMAGES, file);
    return true;
}

static bool web_unload_file() {
    cpc_remove_disc(&state.cpc);
    return true;
}

static void web_save_snapshot(size_t slot_index) {
    ui_save_snapshot(slot_index);
}

static bool web_load_snapshot(size_t slot_index) {
    return ui_load_snapshot(slot_index);
}

static void web_input(const char* text) {
    keybuf_put(text);
}

static void web_dbg_add_breakpoint(uint16_t addr) {
    ui_dbg_add_breakpoint(&state.ui.dbg, addr);
}

static void web_dbg_remove_breakpoint(uint16_t addr) {
    ui_dbg_remove_breakpoint(&state.ui.dbg, addr);
}

static void web_dbg_break(void) {
    ui_dbg_break(&state.ui.dbg);
}

static void web_dbg_continue(void) {
    ui_dbg_continue(&state.ui.dbg, false);
}

static void web_dbg_step_next(void) {
    ui_dbg_step_next(&state.ui.dbg);
}

static void web_dbg_step_into(void) {
    ui_dbg_step_into(&state.ui.dbg);
}

static void web_dbg_on_stopped(int stop_reason, uint16_t addr) {
    // stopping on the entry or exit breakpoints always
    // overrides the incoming stop_reason
    int webapi_stop_reason = WEBAPI_STOPREASON_UNKNOWN;
    if ((state.dbg.entry_addr + 1) == state.cpc.cpu.pc) {
        webapi_stop_reason = WEBAPI_STOPREASON_ENTRY;
        state.dbg.entered = true;
    } else if ((state.dbg.exit_addr + 1) == state.cpc.cpu.pc) {
        // NOTE: the exit address will be hit multiple times before
        // the entry address is actually reached, ignore the exit breakpoint
        // until entry happens
        if (!state.dbg.entered) {
            ui_dbg_continue(&state.ui.dbg, false);
            return;
        }
        webapi_stop_reason = WEBAPI_STOPREASON_EXIT;
    } else if (stop_reason == UI_DBG_STOP_REASON_BREAK) {
        webapi_stop_reason = WEBAPI_STOPREASON_BREAK;
    } else if (stop_reason == UI_DBG_STOP_REASON_STEP) {
        webapi_stop_reason = WEBAPI_STOPREASON_STEP;
    } else if (stop_reason == UI_DBG_STOP_REASON_BREAKPOINT) {
        webapi_stop_reason = WEBAPI_STOPREASON_BREAKPOINT;
    }
    webapi_event_stopped(webapi_stop_reason, addr);
}

static void web_dbg_on_continued(void) {
    webapi_event_continued();
}

static void web_dbg_on_reboot(void) {
    webapi_event_reboot();
}

static void web_dbg_on_reset(void) {
    webapi_event_reset();
}

static webapi_cpu_state_t web_dbg_cpu_state(void) {
    const z80_t* cpu = &state.cpc.cpu;
    return (webapi_cpu_state_t){
        .items = {
            [WEBAPI_CPUSTATE_TYPE] = WEBAPI_CPUTYPE_Z80,
            [WEBAPI_CPUSTATE_Z80_AF]   = cpu->af,
            [WEBAPI_CPUSTATE_Z80_BC]   = cpu->bc,
            [WEBAPI_CPUSTATE_Z80_DE]   = cpu->de,
            [WEBAPI_CPUSTATE_Z80_HL]   = cpu->hl,
            [WEBAPI_CPUSTATE_Z80_IX]   = cpu->ix,
            [WEBAPI_CPUSTATE_Z80_IY]   = cpu->iy,
            [WEBAPI_CPUSTATE_Z80_SP]   = cpu->sp,
            [WEBAPI_CPUSTATE_Z80_PC]   = cpu->pc,
            [WEBAPI_CPUSTATE_Z80_AF2]  = cpu->af2,
            [WEBAPI_CPUSTATE_Z80_BC2]  = cpu->bc2,
            [WEBAPI_CPUSTATE_Z80_DE2]  = cpu->de2,
            [WEBAPI_CPUSTATE_Z80_HL2]  = cpu->hl2,
            [WEBAPI_CPUSTATE_Z80_IM]   = cpu->im,
            [WEBAPI_CPUSTATE_Z80_IR]   = cpu->ir,
            [WEBAPI_CPUSTATE_Z80_IFF]  = (cpu->iff1 ? 1 : 0) | (cpu->iff2 ? 2 : 0),
        }
    };
}

static void web_dbg_request_disassemly(uint16_t addr, int offset_lines, int num_lines, webapi_dasm_line_t* result) {
    assert(num_lines > 0);
    ui_dbg_dasm_line_t* lines = calloc((size_t)num_lines, sizeof(ui_dbg_dasm_line_t));
    ui_dbg_disassemble(&state.ui.dbg, &(ui_dbg_dasm_request_t){
        .addr = addr,
        .num_lines = num_lines,
        .offset_lines = offset_lines,
        .out_lines = lines,
    });
    for (int line_idx = 0; line_idx < num_lines; line_idx++) {
        const ui_dbg_dasm_line_t* src = &lines[line_idx];
        webapi_dasm_line_t* dst = &result[line_idx];
        dst->addr = src->addr;
        dst->num_bytes = (src->num_bytes <= WEBAPI_DASM_LINE_MAX_BYTES) ? src->num_bytes : WEBAPI_DASM_LINE_MAX_BYTES;
        dst->num_chars = (src->num_chars <= WEBAPI_DASM_LINE_MAX_CHARS) ? src->num_chars : WEBAPI_DASM_LINE_MAX_CHARS;
        memcpy(dst->bytes, src->bytes, dst->num_bytes);
        memcpy(dst->chars, src->chars, dst->num_chars);
    }
    free(lines);
}

static void web_dbg_read_memory(uint16_t addr, int num_bytes, uint8_t* dst_ptr) {
    for (int i = 0; i < num_bytes; i++) {
        *dst_ptr++ = mem_rd(&state.cpc.mem, addr++);
    }
}
#endif

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    const chips_display_info_t info = cpc_display_info(0);
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = info.screen.width + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * info.screen.height + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "CPC",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
        .html5_bubble_mouse_events = true,
        .html5_update_document_title = true,
        .logger.func = slog_func,
    };
}

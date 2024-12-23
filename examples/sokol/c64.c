/*
    c64.c
*/
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "common.h"
#include "chips/m6502.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/c1530.h"
#include "chips/m6522.h"
#include "systems/c1541.h"
#include "systems/c64.h"
#include "c64-roms.h"
#include "c1541-roms.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_M6502
    #include "ui.h"
    #include "ui/ui_settings.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_m6502.h"
    #include "ui/ui_m6526.h"
    #include "ui/ui_m6581.h"
    #include "ui/ui_m6569.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_display.h"
    #include "ui/ui_kbd.h"
    #include "ui/ui_snapshot.h"
    #include "ui/ui_c64.h"
#endif
#include <stdlib.h>

typedef struct {
    uint32_t version;
    c64_t c64;
} c64_snapshot_t;

static struct {
    c64_t c64;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    #ifdef CHIPS_USE_UI
        ui_c64_t ui;
        struct {
            uint32_t entry_addr;
            uint32_t exit_addr;
        } dbg;
        c64_snapshot_t snapshots[UI_SNAPSHOT_MAX_SLOTS];
    #endif
} state;

#ifdef CHIPS_USE_UI
static void ui_draw_cb(const ui_draw_info_t* draw_info);
static void ui_save_settings_cb(ui_settings_t* settings);
static void ui_boot_cb(c64_t* sys);
static void ui_save_snapshot(size_t slot_index);
static bool ui_load_snapshot(size_t slot_index);
static void ui_load_snapshots_from_storage(void);
static void web_boot(void);
static void web_reset(void);
static bool web_ready(void);
static bool web_load(chips_range_t data);
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
#define BORDER_TOP (24)
#else
#define BORDER_TOP (8)
#endif
#define BORDER_LEFT (8)
#define BORDER_RIGHT (8)
#define BORDER_BOTTOM (16)
#define LOAD_DELAY_FRAMES (180)

// audio-streaming callback
static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

// get c64_desc_t struct based on joystick type
c64_desc_t c64_desc(c64_joystick_type_t joy_type, bool c1530_enabled, bool c1541_enabled) {
    return (c64_desc_t) {
        .c1530_enabled = c1530_enabled,
        .c1541_enabled = c1541_enabled,
        .joystick_type = joy_type,
        .audio = {
            .callback = { .func = push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .roms = {
            .chars = { .ptr=dump_c64_char_bin, .size=sizeof(dump_c64_char_bin) },
            .basic = { .ptr=dump_c64_basic_bin, .size=sizeof(dump_c64_basic_bin) },
            .kernal = { .ptr=dump_c64_kernalv3_bin, .size=sizeof(dump_c64_kernalv3_bin) },
            .c1541 = {
                .c000_dfff = { .ptr=dump_1541_c000_325302_01_bin, .size=sizeof(dump_1541_c000_325302_01_bin) },
                .e000_ffff = { .ptr=dump_1541_e000_901229_06aa_bin, .size=sizeof(dump_1541_e000_901229_06aa_bin) },
            }
        },
        #if defined(CHIPS_USE_UI)
        .debug = ui_c64_get_debug(&state.ui)
        #endif
    };
}

void app_init(void) {
    saudio_setup(&(saudio_desc){
        .logger.func = slog_func,
    });
    c64_joystick_type_t joy_type = C64_JOYSTICKTYPE_NONE;
    if (sargs_exists("joystick")) {
        if (sargs_equals("joystick", "digital_1")) {
            joy_type = C64_JOYSTICKTYPE_DIGITAL_1;
        } else if (sargs_equals("joystick", "digital_2")) {
            joy_type = C64_JOYSTICKTYPE_DIGITAL_2;
        } else if (sargs_equals("joystick", "digital_12")) {
            joy_type = C64_JOYSTICKTYPE_DIGITAL_12;
        }
    }
    bool c1530_enabled = sargs_exists("c1530");
    bool c1541_enabled = sargs_exists("c1541");
    c64_desc_t desc = c64_desc(joy_type, c1530_enabled, c1541_enabled);
    c64_init(&state.c64, &desc);
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
        .display_info = c64_display_info(&state.c64),
    });
    keybuf_init(&(keybuf_desc_t){ .key_delay_frames=5 });
    clock_init();
    prof_init();
    fs_init();
    #ifdef CHIPS_USE_UI
        ui_init(&(ui_desc_t){
            .draw_cb = ui_draw_cb,
            .save_settings_cb = ui_save_settings_cb,
            .imgui_ini_key = "floooh.chips.c64",
        });
        ui_c64_init(&state.ui, &(ui_c64_desc_t){
            .c64 = &state.c64,
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
                },
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
        ui_c64_load_settings(&state.ui, ui_settings());
        ui_load_snapshots_from_storage();
        // important: initialize webapi after ui
        webapi_init(&(webapi_desc_t){
            .funcs = {
                .boot = web_boot,
                .reset = web_reset,
                .ready = web_ready,
                .load = web_load,
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
    if (sargs_exists("prg")) {
        fs_load_base64(FS_CHANNEL_IMAGES, "url.prg", sargs_value("prg"));
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
    state.ticks = c64_exec(&state.c64, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(c64_display_info(&state.c64));
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
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c > 0x20) && (c < 0x7F)) {
                // need to invert case (unshifted is upper caps, shifted is lower caps
                if (isupper(c)) {
                    c = tolower(c);
                } else if (islower(c)) {
                    c = toupper(c);
                }
                c64_key_down(&state.c64, c);
                c64_key_up(&state.c64, c);
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
                    c64_key_down(&state.c64, c);
                } else {
                    c64_key_up(&state.c64, c);
                }
            }
            break;
        default:
            break;
    }
}

void app_cleanup(void) {
    c64_discard(&state.c64);
    #ifdef CHIPS_USE_UI
        ui_c64_discard(&state.ui);
        ui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

static void send_keybuf_input(void) {
    uint8_t key_code;
    if (0 != (key_code = keybuf_get(state.frame_time_us))) {
        /* FIXME: this is ugly */
        c64_joystick_type_t joy_type = state.c64.joystick_type;
        state.c64.joystick_type = C64_JOYSTICKTYPE_NONE;
        c64_key_down(&state.c64, key_code);
        c64_key_up(&state.c64, key_code);
        state.c64.joystick_type = joy_type;
    }
}

static void handle_file_loading(void) {
    fs_dowork();
    const uint32_t load_delay_frames = LOAD_DELAY_FRAMES;
    if (fs_success(FS_CHANNEL_IMAGES) && clock_frame_count_60hz() > load_delay_frames) {
        bool load_success = false;
        if (fs_ext(FS_CHANNEL_IMAGES, "txt") || fs_ext(FS_CHANNEL_IMAGES, "bas")) {
            load_success = true;
            keybuf_put((const char*)fs_data(FS_CHANNEL_IMAGES).ptr);
        } else if (fs_ext(FS_CHANNEL_IMAGES, "tap")) {
            load_success = c64_insert_tape(&state.c64, fs_data(FS_CHANNEL_IMAGES));
        } else if (fs_ext(FS_CHANNEL_IMAGES, "bin") || fs_ext(FS_CHANNEL_IMAGES, "prg") || fs_ext(FS_CHANNEL_IMAGES, "")) {
            load_success = c64_quickload(&state.c64, fs_data(FS_CHANNEL_IMAGES));
        }
        if (load_success) {
            if (clock_frame_count_60hz() > (load_delay_frames + 10)) {
                gfx_flash_success();
            }
            if (fs_ext(FS_CHANNEL_IMAGES, "tap")) {
                c64_tape_play(&state.c64);
            }
            if (!sargs_exists("debug")) {
                if (sargs_exists("input")) {
                    keybuf_put(sargs_value("input"));
                } else if (fs_ext(FS_CHANNEL_IMAGES, "tap")) {
                    c64_basic_load(&state.c64);
                } else if (fs_ext(FS_CHANNEL_IMAGES, "prg")) {
                    c64_basic_run(&state.c64);
                }
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
    const float w = sapp_widthf();
    const float h = sapp_heightf();
    sdtx_canvas(w, h);
    sdtx_color3b(255, 255, 255);
    sdtx_pos(1.0f, (h / 8.0f) - 1.5f);
    sdtx_printf("frame:%.2fms emu:%.2fms (min:%.2fms max:%.2fms) ticks:%d", (float)state.frame_time_us * 0.001f, emu_stats.avg_val, emu_stats.min_val, emu_stats.max_val, state.ticks);
}

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(const ui_draw_info_t* draw_info) {
    ui_c64_draw(&state.ui, &(ui_c64_frame_t){
        .display = draw_info->display,
    });
}

static void ui_save_settings_cb(ui_settings_t* settings) {
    ui_c64_save_settings(&state.ui, settings);
}

static void ui_boot_cb(c64_t* sys) {
    clock_init();
    c64_desc_t desc = c64_desc(sys->joystick_type, sys->c1530.valid, sys->c1541.valid);
    c64_init(sys, &desc);
}

static void ui_update_snapshot_screenshot(size_t slot) {
    ui_snapshot_screenshot_t screenshot = {
        .texture = ui_create_screenshot_texture(c64_display_info(&state.snapshots[slot].c64))
    };
    ui_snapshot_screenshot_t prev_screenshot = ui_snapshot_set_screenshot(&state.ui.snapshot, slot, screenshot);
    if (prev_screenshot.texture) {
        ui_destroy_texture(prev_screenshot.texture);
    }
}

static void ui_save_snapshot(size_t slot) {
    if (slot < UI_SNAPSHOT_MAX_SLOTS) {
        state.snapshots[slot].version = c64_save_snapshot(&state.c64, &state.snapshots[slot].c64);
        ui_update_snapshot_screenshot(slot);
        fs_save_snapshot("c64", slot, (chips_range_t){ .ptr = &state.snapshots[slot], sizeof(c64_snapshot_t) });
    }
}

static bool ui_load_snapshot(size_t slot) {
    bool success = false;
    if ((slot < UI_SNAPSHOT_MAX_SLOTS) && (state.ui.snapshot.slots[slot].valid)) {
        success = c64_load_snapshot(&state.c64, state.snapshots[slot].version, &state.snapshots[slot].c64);
    }
    return success;
}

static void ui_fetch_snapshot_callback(const fs_snapshot_response_t* response) {
    assert(response);
    if (response->result != FS_RESULT_SUCCESS) {
        return;
    }
    if (response->data.size != sizeof(c64_snapshot_t)) {
        return;
    }
    if (((c64_snapshot_t*)response->data.ptr)->version != C64_SNAPSHOT_VERSION) {
        return;
    }
    size_t snapshot_slot = response->snapshot_index;
    assert(snapshot_slot < UI_SNAPSHOT_MAX_SLOTS);
    memcpy(&state.snapshots[snapshot_slot], response->data.ptr, response->data.size);
    ui_update_snapshot_screenshot(snapshot_slot);
}

static void ui_load_snapshots_from_storage(void) {
    for (size_t snapshot_slot = 0; snapshot_slot < UI_SNAPSHOT_MAX_SLOTS; snapshot_slot++) {
        fs_load_snapshot_async("c64", snapshot_slot, ui_fetch_snapshot_callback);
    }
}

// webapi wrappers
static void web_boot(void) {
    clock_init();
    c64_desc_t desc = c64_desc(state.c64.joystick_type, state.c64.c1530.valid, state.c64.c1541.valid);
    c64_init(&state.c64, &desc);
    ui_dbg_reboot(&state.ui.dbg);
}

static void web_reset(void) {
    c64_reset(&state.c64);
    ui_dbg_reset(&state.ui.dbg);
}

static void web_dbg_connect(void) {
    gfx_disable_speaker_icon();
    state.dbg.entry_addr = 0xFFFFFFFF;
    state.dbg.exit_addr = 0xFFFFFFFF;
    ui_dbg_external_debugger_connected(&state.ui.dbg);
}

static void web_dbg_disconnect(void) {
    state.dbg.entry_addr = 0xFFFFFFFF;
    state.dbg.exit_addr = 0xFFFFFFFF;
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
    if ((hdr->type[0] != 'P') || (hdr->type[1] != 'R') || (hdr->type[2] != 'G') || (hdr->type[3] != ' ')) {
        return false;
    }
    const uint16_t start_addr = (hdr->start_addr_hi<<8) | hdr->start_addr_lo;
    const chips_range_t prg = { .ptr = (void*)&hdr->payload, .size = data.size - sizeof(webapi_fileheader_t) };
    bool loaded = c64_quickload(&state.c64, prg);
    if (loaded) {
        state.dbg.entry_addr = start_addr;
        state.dbg.exit_addr = c64_syscall_return_addr();
        ui_dbg_add_breakpoint(&state.ui.dbg, state.dbg.entry_addr);
        ui_dbg_add_breakpoint(&state.ui.dbg, state.dbg.exit_addr);
        // if debugger is stopped, unstuck it
        if (ui_dbg_stopped(&state.ui.dbg)) {
            ui_dbg_continue(&state.ui.dbg, false);
        }
        // execute a SYS start_addr
        c64_basic_syscall(&state.c64, start_addr);
    }
    return loaded;
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
    if (state.dbg.entry_addr == state.c64.cpu.PC) {
        webapi_stop_reason = WEBAPI_STOPREASON_ENTRY;
    } else if (state.dbg.exit_addr == state.c64.cpu.PC) {
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
    const m6502_t* cpu = &state.c64.cpu;
    return (webapi_cpu_state_t){
        .items = {
            [WEBAPI_CPUSTATE_TYPE] = WEBAPI_CPUTYPE_6502,
            [WEBAPI_CPUSTATE_6502_A] = cpu->A,
            [WEBAPI_CPUSTATE_6502_X] = cpu->X,
            [WEBAPI_CPUSTATE_6502_Y] = cpu->Y,
            [WEBAPI_CPUSTATE_6502_S] = cpu->S,
            [WEBAPI_CPUSTATE_6502_P] = cpu->P,
            [WEBAPI_CPUSTATE_6502_PC] = cpu->PC,
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
        *dst_ptr++ = mem_rd(&state.c64.mem_cpu, addr++);
    }
}
#endif

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){
        .argc=argc,
        .argv=argv,
        .buf_size = 512 * 1024,
    });
    const chips_display_info_t info = c64_display_info(0);
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * info.screen.width + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * info.screen.height + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "C64",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
        .html5_bubble_mouse_events = true,
        .logger.func = slog_func,
    };
}

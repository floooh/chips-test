/*
    cpc.c

    Amstrad CPC 464/6128 and KC Comptact. No disc emulation.
*/
#include "sokol_app.h"
#include "sokol_audio.h"
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
#include "common/common.h"
#include "roms/cpc-roms.h"

cpc_t cpc;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    args_init(argc, argv);
    fs_init();
    if (args_has("file")) {
        fs_load_file(args_string("file"));
    }
    if (args_has("tape")) {
        /* FIXME: TAPE LOADING */
    }
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

/* one-time application init */
void app_init(void) {
    gfx_init(CPC_DISPLAY_WIDTH, CPC_DISPLAY_HEIGHT, 1, 2);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    cpc_type_t type = CPC_TYPE_6128;
    if (args_has("type")) {
        if (args_string_compare("type", "cpc464")) {
            type = CPC_TYPE_464;
        }
        else if (args_string_compare("type", "kccompact")) {
            type = CPC_TYPE_KCCOMPACT;
        }
    }
    cpc_joystick_t joy_type = CPC_JOYSTICK_NONE;
    if (args_has("joystick")) {
        joy_type = CPC_JOYSTICK_DIGITAL;
    }
    cpc_init(&cpc, &(cpc_desc_t){
        .type = type,
        .joystick_type = joy_type,
        .pixel_buffer = rgba8_buffer,
        .pixel_buffer_size = sizeof(rgba8_buffer),
        .audio_cb = saudio_push,
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
    /* FIXME
    if (fs_ptr() && clock_frame_count() > 20) {
        cpc_load_sna(cpc, fs_ptr(), fs_size());
        fs_free();
    }
    */
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if (c < KBD_MAX_KEYS) {
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

/* CPC SNA fileformat header: http://cpctech.cpc-live.com/docs/snapshot.html */
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

bool cpc_load_sna(cpc_t* sys, const uint8_t* ptr, uint32_t num_bytes) {
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
    if (dump_num_bytes > sizeof(sys->ram)) {
        return false;
    }
    memcpy(sys->ram, ptr, dump_num_bytes);

    z80_reset(&sys->cpu);
    z80_set_f(&sys->cpu, hdr->F); z80_set_a(&sys->cpu,hdr->A);
    z80_set_c(&sys->cpu, hdr->C); z80_set_b(&sys->cpu, hdr->B);
    z80_set_e(&sys->cpu, hdr->E); z80_set_d(&sys->cpu, hdr->D);
    z80_set_l(&sys->cpu, hdr->L); z80_set_h(&sys->cpu, hdr->H);
    z80_set_r(&sys->cpu, hdr->R); z80_set_i(&sys->cpu, hdr->I);
    z80_set_iff1(&sys->cpu, (hdr->IFF1 & 1) != 0);
    z80_set_iff2(&sys->cpu, (hdr->IFF2 & 1) != 0);
    z80_set_ix(&sys->cpu, hdr->IX_h<<8 | hdr->IX_l);
    z80_set_iy(&sys->cpu, hdr->IY_h<<8 | hdr->IY_l);
    z80_set_sp(&sys->cpu, hdr->SP_h<<8 | hdr->SP_l);
    z80_set_pc(&sys->cpu, hdr->PC_h<<8 | hdr->PC_l);
    z80_set_im(&sys->cpu, hdr->IM);
    z80_set_af_(&sys->cpu, hdr->A_<<8 | hdr->F_);
    z80_set_bc_(&sys->cpu, hdr->B_<<8 | hdr->C_);
    z80_set_de_(&sys->cpu, hdr->D_<<8 | hdr->E_);
    z80_set_hl_(&sys->cpu, hdr->H_<<8 | hdr->L_);

    for (int i = 0; i < 16; i++) {
        sys->ga.palette[i] = sys->ga.colors[hdr->pens[i] & 0x1F];
    }
    sys->ga.border_color = sys->ga.colors[hdr->pens[16] & 0x1F];
    sys->ga.pen = hdr->selected_pen & 0x1F;
    sys->ga.config = hdr->gate_array_config & 0x3F;
    sys->ga.next_video_mode = hdr->gate_array_config & 3;
    sys->ga.ram_config = hdr->ram_config & 0x3F;
    sys->upper_rom_select = hdr->rom_config;
    _cpc_update_memory_mapping(sys);

    for (int i = 0; i < 18; i++) {
        sys->vdg.reg[i] = hdr->crtc_regs[i];
    }
    sys->vdg.sel = hdr->crtc_selected;

    sys->ppi.output[I8255_PORT_A] = hdr->ppi_a;
    sys->ppi.output[I8255_PORT_B] = hdr->ppi_b;
    sys->ppi.output[I8255_PORT_C] = hdr->ppi_c;
    sys->ppi.control = hdr->ppi_control;

    for (int i = 0; i < 16; i++) {
        ay38910_iorq(&sys->psg, AY38910_BDIR|AY38910_BC1|(i<<16));
        ay38910_iorq(&sys->psg, AY38910_BDIR|(hdr->psg_regs[i]<<16));
    }
    ay38910_iorq(&sys->psg, AY38910_BDIR|AY38910_BC1|(hdr->psg_selected<<16));
    return true;
}

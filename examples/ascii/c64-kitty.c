//------------------------------------------------------------------------------
//  c64-kitty.c
//
//  A C64 emulator for the terminal, using the Kitty graphics protocol:
//
//  https://sw.kovidgoyal.net/kitty/graphics-protocol/
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#define SOKOL_IMPL
#include "sokol_time.h"
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "chips/m6502.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/c1530.h"
#include "chips/m6522.h"
#include "systems/c1541.h"
#include "systems/c64.h"
#include "c64-roms.h"
#include "kitty.h"

#define FRAME_USEC (33333)

static struct {
    c64_t c64;
    chips_range_t prg;
    uint8_t prg_buf[(1<<16)];
} state;

// a signal handler for Ctrl-C, for proper cleanup
static int quit_requested = 0;
static void catch_sigint(int signo) {
    (void)signo;
    quit_requested = 1;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        FILE* fp = fopen(argv[1], "rb");
        if (fp) {
            size_t bytes_read = fread(state.prg_buf, 1, sizeof(state.prg_buf), fp);
            fclose(fp);
            if (bytes_read > 0) {
                state.prg = (chips_range_t){
                    .ptr = state.prg_buf,
                    .size = bytes_read,
                };
            }
        } else {
            printf("Failed to load '%s'\n", argv[1]);
            exit(10);
        }
    }

    // install a Ctrl-C signal handler
    signal(SIGINT, catch_sigint);

    // sokol time for rendering frames at correct speed
    stm_setup();

    kitty_setup_termios();
    kitty_hide_cursor();

    // setup the C64 emulator
    c64_init(&state.c64, &(c64_desc_t){
        .roms = {
            .chars = { .ptr=dump_c64_char_bin, .size=sizeof(dump_c64_char_bin) },
            .basic = { .ptr=dump_c64_basic_bin, .size=sizeof(dump_c64_basic_bin) },
            .kernal = { .ptr=dump_c64_kernalv3_bin, .size=sizeof(dump_c64_kernalv3_bin) }
        },
    });
    size_t frame_count = 0;
    while (!quit_requested) {
        frame_count += 1;
        uint64_t start = stm_now();

        // run emulation for one frame
        c64_exec(&state.c64, FRAME_USEC);

        /*
        if ((frame_count == 150) && state.prg.ptr) {
            c64_quickload(&state.c64, state.prg);
            state.keybuf.buf[0] = 'R';
            state.keybuf.buf[1] = 'U';
            state.keybuf.buf[2] = 'N';
            state.keybuf.buf[3] = 0x0D;
        }

        // keyboard input
        int ch = getch();
        if (ch != ERR) {
            switch (ch) {
                case 10:  ch = 0x0D; break; // ENTER
                case 127: ch = 0x01; break; // BACKSPACE
                case 27:  ch = 0x03; break; // ESCAPE
                case 260: ch = 0x08; break; // LEFT
                case 261: ch = 0x09; break; // RIGHT
                case 259: ch = 0x0B; break; // UP
                case 258: ch = 0x0A; break; // DOWN
            }
            if (ch > 32) {
                if (islower(ch)) {
                    ch = toupper(ch);
                } else if (isupper(ch)) {
                    ch = tolower(ch);
                }
            }
            if (ch < 256) {
                c64_key_down(&state.c64, ch);
                c64_key_up(&state.c64, ch);
            }
        }
        if (state.keybuf.buf[state.keybuf.index] != 0) {
            if ((frame_count + 5) > state.keybuf.frame_count) {
                state.keybuf.frame_count = frame_count;
                char ch = state.keybuf.buf[state.keybuf.index++];
                c64_key_down(&state.c64, ch);
                c64_key_up(&state.c64, ch);
            }
        }
        */

        // render the frame in the terminal
        uint32_t frame_id = 2 + (frame_count & 1);
        kitty_set_pos(0, 0);
        kitty_send_image(c64_display_info(&state.c64), frame_id);

        kitty_poll_events(FRAME_USEC / 1000);

        // sleep until next frame
        //double dur_usec = stm_us(stm_since(start));
        //if (dur_usec < FRAME_USEC) {
        //    usleep(FRAME_USEC - dur_usec);
        //}
    }
    kitty_poll_events(50);
    kitty_show_cursor();
    kitty_restore_termios();
    return 0;
}

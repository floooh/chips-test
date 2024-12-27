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
#include <curses.h>     // curses is only used for non-blocking keyboard input
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

#define FRAME_USEC (16666)
#define MAX_WIDTH (512)
#define MAX_HEIGHT (512)
#define BYTES_PER_PIXEL (3)

static struct {
    c64_t c64;
    int width;
    int height;
    chips_range_t prg;
    struct {
        char buf[16];
        size_t index;
        size_t frame_count;
    } keybuf;
    uint8_t rgb[MAX_WIDTH * MAX_HEIGHT * BYTES_PER_PIXEL];
    uint8_t b64_buf[MAX_WIDTH * MAX_HEIGHT * BYTES_PER_PIXEL * 2];
    uint8_t io_buf[(1<<16)];
} state;

unsigned char* base64_encode(const uint8_t *src, size_t len, chips_range_t dst, size_t* out_len);

// a signal handler for Ctrl-C, for proper cleanup
static int quit_requested = 0;
static void catch_sigint(int signo) {
    (void)signo;
    quit_requested = 1;
}

// convert the C64 frame buffer to 24-bit RGB
static void convert_framebuffer(void) {
    const chips_display_info_t info = c64_display_info(&state.c64);
    assert((info.screen.width < MAX_WIDTH) && (info.screen.height < MAX_HEIGHT));
    assert(info.palette.ptr && (info.palette.size == 256 * 4));
    assert(info.frame.bytes_per_pixel == 1);
    state.width = info.screen.width;
    state.height = info.screen.height;
    const int src_width = info.frame.dim.width;
    const uint8_t* src_pixels = info.frame.buffer.ptr;
    const uint32_t* pal = info.palette.ptr;
    for (int y = 0; y < state.height; y++) {
        for (int x = 0; x < state.width; x++) {
            int src_idx = y * src_width + x;
            int dst_idx = (y * state.width + x) * BYTES_PER_PIXEL;
            uint32_t rgba = pal[src_pixels[src_idx]];
            state.rgb[dst_idx++] = rgba & 255;
            state.rgb[dst_idx++] = (rgba >> 8) & 255;
            state.rgb[dst_idx++] = (rgba >> 16) & 255;
        }
    }
}

// move cursor back to topleft
static void term_home(void) {
    printf("\033[0;0H");
}

// dump converted framebuffer pixels
static void term_kitty_pixels(void) {
    printf("\033_Ga=T,f=24,s=%d,v=%d,c=80,r=30;", state.width, state.height);
    const int num_bytes = state.width * state.height * BYTES_PER_PIXEL;
    size_t b64_num_bytes = 0;
    chips_range_t out_buf = { .ptr = state.b64_buf, .size = sizeof(state.b64_buf) };
    const unsigned char* b64 = base64_encode(state.rgb, num_bytes, out_buf, &b64_num_bytes);
    fwrite(b64, 1, b64_num_bytes, stdout);
    printf("\033\\");
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        FILE* fp = fopen(argv[1], "rb");
        if (fp) {
            size_t bytes_read = fread(state.io_buf, 1, sizeof(state.io_buf), fp);
            fclose(fp);
            if (bytes_read > 0) {
                state.prg = (chips_range_t){
                    .ptr = state.io_buf,
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

    // setup curses for non-blocking IO
    initscr();
    noecho();
    curs_set(FALSE);
    cbreak();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    // sokol time for rendering frames at correct speed
    stm_setup();

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

        // render the frame in the terminal
        convert_framebuffer();
        term_home();
        term_kitty_pixels();
        fflush(stdout);

        // sleep until next frame
        double dur_usec = stm_us(stm_since(start));
        if (dur_usec < FRAME_USEC) {
            usleep(FRAME_USEC - dur_usec);
        }
    }
    endwin();
    return 0;
}

// from: https://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
static const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char* base64_encode(const uint8_t *src, size_t len, chips_range_t dst, size_t* out_len) {
	unsigned char *out, *pos;
	const unsigned char *end, *in;
	size_t olen;
	int line_len;

	olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	olen += olen / 72; /* line feeds */
	olen++; /* nul termination */
	if (olen < len) {
		return NULL; /* integer overflow */
    }
	out = dst.ptr;
	if (out == NULL) {
		return NULL;
    }

	end = src + len;
	in = src;
	pos = out;
	line_len = 0;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
		line_len += 4;
		if (line_len >= 72) {
			*pos++ = '\n';
			line_len = 0;
		}
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) |
					      (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
		line_len += 4;
	}

	if (line_len)
		*pos++ = '\n';

	*pos = '\0';
	if (out_len) {
		*out_len = pos - out;
    }
    assert(*out_len <= dst.size);
	return out;
}

// NOTE: https://github.com/michaeljclark/glkitty/blob/trunk/src/kitty_util.h

#include "chips/chips_common.h"
#include "kitty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <termios.h>
#include <poll.h>
#include <unistd.h>

// NOTE: can't get chunked upload to work
#define USE_CHUNKED_UPLOAD (0)

static chips_range_t base64_encode(chips_range_t data, chips_range_t dst);

#define MAX_WIDTH (512)
#define MAX_HEIGHT (512)
#define FB_NUM_BYTES (MAX_WIDTH * MAX_HEIGHT * BYTES_PER_PIXEL)

static struct {
    size_t width;
    size_t height;
    uint32_t fb[MAX_WIDTH * MAX_HEIGHT];
    uint8_t b64_buf[MAX_WIDTH * MAX_HEIGHT * 4 * 2];
} state;

typedef struct {
    size_t r;
    char buf[256];
} line_t;

typedef struct {
    int iid;
    size_t offset;
    line_t data;
} kdata_t;

static line_t kitty_recv_term(int timeout) {
    line_t l = {0};
    int r;
    struct pollfd fds[1] = {0};

    fds[0].fd = fileno(stdin);
    fds[0].events = POLLIN;

    if ((r = poll(fds, 1, timeout)) <= 0) {
        return l;
    }
    if ((fds[0].events & POLLIN) == 0) {
        return l;
    }
    l.r = read(fds[0].fd, l.buf, sizeof(l.buf)-1);
    if (l.r > 0) {
        l.buf[l.r] = '\0';
    }
fprintf(stderr, "recv: %s\n", l.buf);
    return l;
}

static kdata_t kitty_parse_response(line_t l) {
    /*
     * parse kitty response of the form: "\x1B_Gi=<image_id>;OK\x1B\\"
     *
     * NOTE: a keypress can be present before or after the data
     */
    if (l.r < 1) {
        return (kdata_t){ .iid = -1, .offset = 0, .data = l };
    }
    char* esc = strchr(l.buf, '\e');
    if (!esc) {
        return (kdata_t){ .iid = -1, .offset = 0, .data = l };
    }
    ptrdiff_t offset = (esc - l.buf) + 1;
    int iid = 0;
    int r = sscanf(l.buf + offset, "_Gi=%d;OK", &iid);
    if (r != 1) {
        return (kdata_t){ .iid = -1, .offset = 0, .data = l };
    }
    return (kdata_t){ .iid = iid, .offset = offset, .data = l };
}

static struct termios* _get_termios_backup(void) {
    static struct termios backup;
    return &backup;
}

static struct termios* _get_termios_raw(void) {
    static struct termios raw;
    cfmakeraw(&raw);
    return &raw;
}

void kitty_setup_termios(void) {
    tcgetattr(0, _get_termios_backup());
    tcsetattr(0, TCSANOW, _get_termios_raw());
}

void kitty_restore_termios(void) {
    tcsetattr(0, TCSANOW, _get_termios_backup());
}

void kitty_hide_cursor(void) {
    puts("\e[?25l");
}

void kitty_show_cursor(void) {
    puts("\e[?25h");
}

void kitty_set_pos(int x, int y) {
    printf("\e[%d;%dH", y, x);
    fflush(stdout);
}

bool kitty_send_image(chips_display_info_t disp_info, uint32_t frame_id) {
    assert(disp_info.screen.width <= MAX_WIDTH);
    assert(disp_info.screen.height <= MAX_HEIGHT);
    assert(disp_info.frame.bytes_per_pixel == 1);
    assert(disp_info.palette.ptr && (disp_info.palette.size == 256 * sizeof(uint32_t)));

    const size_t width = disp_info.screen.width;
    const size_t height = disp_info.screen.height;

    // palette decode
    const uint8_t* src = disp_info.frame.buffer.ptr;
    const size_t src_pitch = disp_info.frame.dim.width;
    const uint32_t* pal = disp_info.palette.ptr;
    uint32_t* dst = state.fb;
    for (size_t y = 0; y < height; y++, src += src_pitch) {
        for (size_t x = 0; x < width; x++, dst++) {
            *dst = pal[src[x]];
        }
    }

    // base64-encode
    const chips_range_t b64_src = { .ptr = state.fb, .size = width * height * sizeof(uint32_t) };
    const chips_range_t b64_dst = { .ptr = state.b64_buf, .size = sizeof(state.b64_buf) };
    chips_range_t b64 = base64_encode(b64_src, b64_dst);
    if (b64.ptr == 0) {
        return false;
    }

    #if USE_CHUNKED_UPLOAD
        const size_t chunk_limit = 4096;
        size_t sent_bytes = 0;
        const uint8_t* b64_ptr = b64.ptr;
        while (sent_bytes < b64.size) {
            size_t chunk_size = (b64.size - sent_bytes) < chunk_limit ? b64.size - sent_bytes : chunk_limit;
            int cont = !!(sent_bytes + chunk_size < b64.size);
            if (sent_bytes == 0) {
                fprintf(stdout, "\e_Ga=T,f=32,i=%u,s=%zu,v=%zu,m=%d;", frame_id, width, height, cont);
            } else {
                fprintf(stdout, "\e_Gm=%d;", cont);
            }
            fwrite(b64_ptr + sent_bytes, chunk_size, 1, stdout);
            fprintf(stdout, "\e\\");
            sent_bytes += chunk_size;
        }
    #else
        fprintf(stdout, "\e_Ga=T,t=d,f=32,i=%u,s=%zu,v=%zu;", frame_id, width, height);
        fwrite(b64.ptr, b64.size, 1, stdout);
        fprintf(stdout, "\e\\");
    #endif

    fflush(stdout);
    return true;
}

void kitty_poll_events(int ms) {
    kdata_t k;
    do {
        k = kitty_parse_response(kitty_recv_term(ms));
        // FIXME: check key pressed
    } while (k.iid > 0);
}

// from: https://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
static const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static chips_range_t base64_encode(chips_range_t data, chips_range_t dst) {
    const uint8_t* src = data.ptr;
    size_t len = data.size;
	size_t olen = len * 4 / 3 + 4; // 3-byte blocks to 4-byte
	olen += olen / 72; // line feeds
	olen++; // nul termination
	if (olen < len) {
		return (chips_range_t){0};  // integer overflow
    }
    if (olen > dst.size) {
        return (chips_range_t){0};  // output buffer too small
    }
	unsigned char* out = dst.ptr;
	if (out == 0) {
		return (chips_range_t){0};
    }

	const uint8_t* end = src + len;
	const uint8_t* in = src;
	unsigned char* pos = out;
	int line_len = 0;
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

	if (line_len) {
		*pos++ = '\n';
    }
	*pos = '\0';
    return (chips_range_t){ .ptr = dst.ptr, .size = pos - out };
}

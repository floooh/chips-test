//------------------------------------------------------------------------------
//  png2bits.c
//
//  Convert a PNG with alpha channel into a C array with one bit per pixel.
//
//  Usage:
//  fips run png2bits --input image.png --output image.h --cname bla
//------------------------------------------------------------------------------
#include <stdint.h>
#include "getopt.h"
#define STB_IMAGE_IMPLEMENTATION
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"

static const struct getopt_option option_list[] = {
    { "help", 'h', GETOPT_OPTION_TYPE_NO_ARG, 0, 'h', "print this help text", 0},
    { "input", 'i', GETOPT_OPTION_TYPE_REQUIRED, 0, 'i', "input PNG filename", 0},
    { "output", 'o', GETOPT_OPTION_TYPE_REQUIRED, 0, 'o', "output C header filename", 0},
    { "cname", 'c', GETOPT_OPTION_TYPE_REQUIRED, 0, 'c', "C name of generated data", 0},
    GETOPT_OPTIONS_END
};

static char help_buf[2048];

static uint8_t pack_pixels(const uint8_t* pixels, int num_pixels) {
    uint8_t p = 0;
    for (int i = 0; i < num_pixels; i++) {
        if (pixels[3] != 0) {
            p |= (1<<i);
        }
        pixels += 4;
    }
    return p;
}

int main(int argc, const char** argv) {
    getopt_context_t ctx;
    if (getopt_create_context(&ctx, argc, argv, option_list) < 0) {
        fprintf(stderr, "getopt_create_contex() failed!\n");
        return 10;
    }
    const char* inp_path = 0;
    const char* out_path = 0;
    const char* cname = 0;
    int opt;
    while (((opt = getopt_next(&ctx)) != -1)) {
        switch (opt) {
            case '+':
                fprintf(stderr, "get argument without flag: %s\n", ctx.current_opt_arg);
                return 10;
            case '?':
                fprintf(stderr, "unknown flag %s\n", ctx.current_opt_arg);
                return 10;
            case '!':
                fprintf(stderr, "invalid use of flag %s\n", ctx.current_opt_arg);
                return 10;
            case 'h':
                fprintf(stderr, "png2bits -- convert PNG into packaged bit array in C cheader\n\n");
                fprintf(stderr, "%s", getopt_create_help_string(&ctx, help_buf, sizeof(help_buf)));
                return 0;
            case 'i':
                inp_path = ctx.current_opt_arg;
                break;
            case 'o':
                out_path = ctx.current_opt_arg;
                break;
            case 'c':
                cname = ctx.current_opt_arg;
                break;
            default:
                break;
        }
    }
    if (0 == inp_path) {
        fprintf(stderr, "input png file expected (--input, -i)\n");
        return 10;
    }
    if (0 == out_path) {
        fprintf(stderr, "output C header name expected (--output, -o)\n");
        return 10;
    }
    if (0 == cname) {
        fprintf(stderr, "C name expected (--cname, -c)\n");
        return 10;
    }
    
    int width, height, num_comps;
    const uint8_t* pixels = stbi_load(inp_path, &width, &height, &num_comps, 4);
    if (pixels) {
        FILE* fp = fopen(out_path, "w");
        if (!fp) {
            fprintf(stderr, "failed to open output file %s\n", out_path);
            return 10;
        }
        
        const int stride = (width + 7) / 8;
        fprintf(fp, "#pragma once\n");
        fprintf(fp, "static const struct {\n");
        fprintf(fp, "    int width;\n");
        fprintf(fp, "    int height;\n");
        fprintf(fp, "    int stride;\n");
        fprintf(fp, "    uint8_t pixels[%d];\n", height * stride);
        fprintf(fp, "} %s = {\n", cname);
        fprintf(fp, "    .width = %d,\n", width);
        fprintf(fp, "    .height = %d,\n", height);
        fprintf(fp, "    .stride = %d,\n", stride);
        fprintf(fp, "    .pixels = {\n        ");
        for (int y = 0; y < height; y++) {
            const int num_packed_bytes = width >> 3;
            const int num_remainder_bits = width & 7;
            for (int x = 0; x < num_packed_bytes; x++) {
                uint8_t p = pack_pixels(pixels, 8);
                fprintf(fp, "0x%02X,", p);
                pixels += 8 * 4;
            }
            if (num_remainder_bits > 0) {
                uint8_t p = pack_pixels(pixels, num_remainder_bits);
                fprintf(fp, "0x%02X,", p);
                pixels += num_remainder_bits * 4;
            }
            fprintf(fp, "\n");
            if (y < (height - 1)) {
                fprintf(fp, "        ");
            }
        }
        fprintf(fp, "\n    }\n");
        fprintf(fp, "};\n");
    }
    else {
        fprintf(stderr, "failed to load image %s\n", inp_path);
        return 10;
    }
    // success
    return 0;
}

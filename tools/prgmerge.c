//------------------------------------------------------------------------------
//  prgmerge.c
//
//  Merge two PRG files into one, useful for creating a single PRG file from
//  VIC-20 game cartridge PRGs (which are usually two PRGs at two different
//  addresses).
//
//  Note that loading such a merged PRG must protect the 'gap' bytes from
//  being overwritten!
//
//  Usage:
//
//  fips run prgmerge -- [--first first.prg] [--second second.prg [--output output.prg]
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "getopt.h"

static const struct getopt_option option_list[] = {
    { "help", 'h', GETOPT_OPTION_TYPE_NO_ARG, 0, 'h', "print this help text", 0},
    { "first", 'f', GETOPT_OPTION_TYPE_REQUIRED, 0, 'f', "first input .PRG file", "first.prg"},
    { "second", 's', GETOPT_OPTION_TYPE_REQUIRED, 0, 's', "second input .PRG file", "second.prg"},
    { "output", 'o', GETOPT_OPTION_TYPE_REQUIRED, 0, 'o', "merged output .PRG file", "output.prg"},
    GETOPT_OPTIONS_END
};

char help_buf[2048];

typedef struct {
    uint16_t start;
    uint16_t end;
} range_t;

typedef struct {
    const char* path;
    FILE* fp;
    int len;
    uint8_t* buf;
    range_t range;
} file_t;

file_t inp0, inp1, output;
uint8_t mem[1<<16];

int file_length(FILE* fp);
range_t copy_prg(const uint8_t* ptr, int num_bytes);

int main(int argc, const char** argv) {
    
    getopt_context_t ctx;
    if (getopt_create_context(&ctx, argc, argv, option_list) < 0) {
        fprintf(stderr, "getopt_create_contex() failed!\n");
        return 10;
    }
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
                fprintf(stderr, "prgmerge -- merge two .prg files into one\n\n");
                fprintf(stderr, "%s", getopt_create_help_string(&ctx, help_buf, sizeof(help_buf)));
                return 0;
            case 'f':
                inp0.path = ctx.current_opt_arg;
                break;
            case 's':
                inp1.path = ctx.current_opt_arg;
                break;
            case 'o':
                output.path = ctx.current_opt_arg;
                break;
            default:
                break;
        }
    }
    if (!inp0.path) {
        fprintf(stderr, "first input .prg file path expected (--first, -f)\n");
        return 10;
    }
    if (!inp1.path) {
        fprintf(stderr, "second input .prg file path expected (--second, -s)\n");
        return 10;
    }
    if (!output.path) {
        fprintf(stderr, "output file path expected (--output, -o)\n");
        return 10;
    }

    inp0.fp = fopen(inp0.path, "rb");
    inp1.fp = fopen(inp1.path, "rb");
    output.fp = fopen(output.path, "wb");
    if (!inp0.fp) {
        fprintf(stderr, "failed to open input file '%s'\n", inp0.path);
        return 10;
    }
    if (!inp1.fp) {
        fprintf(stderr, "failed to open input file '%s'\n", inp1.path);
        return 10;
    }
    if (!output.fp) {
        fprintf(stderr, "failed to open output file '%s'\n", output.path);
        return 10;
    }

    inp0.len = file_length(inp0.fp);
    inp1.len = file_length(inp0.fp);
    if (inp0.len <= 2) {
        fprintf(stderr, "input file %s not a .prg file (too small)\n", inp0.path);
        return 10;
    }
    if (inp1.len <= 2) {
        fprintf(stderr, "input file %s not a .prg file (too small)\n", inp1.path);
        return 10;
    }
    if (inp0.len >= (1<<16)) {
        fprintf(stderr, "input file %s too big\n", inp0.path);
        return 10;
    }
    if (inp1.len >= (1<<16)) {
        fprintf(stderr, "input file %s too big\n", inp1.path);
        return 10;
    }
    if ((inp0.len + inp1.len) >= (1<<16)) {
        fprintf(stderr, "sum of input file sizes too buf\n");
        return 10;
    }
    inp0.buf = malloc(inp0.len);
    inp1.buf = malloc(inp1.len);
    int res0 = (int) fread(inp0.buf, 1, inp0.len, inp0.fp);
    int res1 = (int) fread(inp1.buf, 1, inp1.len, inp1.fp);
    if (res0 != inp0.len) {
        fprintf(stderr, "failed reading content of input file %s\n", inp0.path);
        return 10;
    }
    if (res1 != inp1.len) {
        fprintf(stderr, "failed reading content of inpue file %s\n", inp1.path);
        return 10;
    }

    memset(mem, 0xFF, sizeof(mem));
    inp0.range = copy_prg(inp0.buf, inp0.len);
    inp1.range = copy_prg(inp1.buf, inp1.len);
    output.range = (range_t) {
        .start = inp0.range.start < inp1.range.start ? inp0.range.start : inp1.range.start,
        .end   = inp0.range.end   > inp1.range.end   ? inp0.range.end   : inp1.range.end
    };
    assert(output.range.start < output.range.end);

    uint8_t lo = output.range.start & 0xFF;
    uint8_t hi = (output.range.start>>8) & 0xFF;
    fwrite(&lo, 1, 1, output.fp);
    fwrite(&hi, 1, 1, output.fp);
    fwrite(&mem[output.range.start], output.range.end - output.range.start, 1, output.fp);

    return 0;
}

int file_length(FILE* fp) {
    fseek(fp, 0, SEEK_END);
    int len = (int) ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return len;
}

range_t copy_prg(const uint8_t* ptr, int num_bytes) {
    assert(num_bytes > 2);
    uint16_t addr = (ptr[1]<<8) | ptr[0];
    ptr += 2;
    num_bytes -= 2;
    for (int i = 0; i < num_bytes; i++) {
        mem[(addr + i) & 0xFFFF] = ptr[i];
    }
    return (range_t) {
        .start = addr,
        .end = addr + num_bytes
    };
}

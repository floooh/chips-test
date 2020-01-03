/*
   A program for displaying lightpen.prg dumps
   2010 Hannu Nuotio
*/

#include <stdio.h>

#define BYTE unsigned char

int analyse(char *filename)
{
    FILE *fp;

    BYTE load_addr[2];
    BYTE header[64];
    BYTE line_lsb[256];
    BYTE line_msb[256];
    BYTE lp_x[256];
    BYTE lp_y[256];

    int i;
    int cycle;
    int cycles_per_line;

    if ((fp = fopen(filename, "r")) == NULL) {
        perror(filename);
        return 1;
    }

    fprintf(stdout, "Processing %s\n", filename);

    if ((fread(load_addr, 1, 2, fp)) != 2) {
        fclose(fp);
        perror(filename);
        return 1;
    }

    if ((load_addr[0] != 0xc0) || (load_addr[1] != 0x17)) {
        fprintf(stdout, "Invalid load address $%02x%02x, skipping.\n", load_addr[1], load_addr[0]);
        fclose(fp);
        return 0;
    }

    if (((fread(header, 1, 64, fp)) != 64) ||
        ((fread(line_lsb, 1, 256, fp)) != 256) ||
        ((fread(line_msb, 1, 256, fp)) != 256) ||
        ((fread(lp_x, 1, 256, fp)) != 256) ||
        ((fread(lp_y, 1, 256, fp)) != 256)) {
        fclose(fp);
        perror(filename);
        return 1;
    }

    fclose(fp);

    cycles_per_line = header[32];
    fprintf(stdout, "Name: %s - Cycles: %u, Lines: %u, Stable: %c\n",
                header, cycles_per_line, header[33] + 256, header[40] ? 'y':'n');

    for (i = 1; i < 256; ++i) {
        if ((line_lsb[i] ^ line_lsb[i-1]) & 0x80) {
            cycle = cycles_per_line - i;
            break;
        }
    }

    for (i = 0; i < 256; ++i) {
        fprintf(stdout, "Test %02x - Line: %03x, Cycle %02i, X: %02x, Y: %02x (%02x)\n",
                i,
                (line_msb[i] << 1) | ((line_lsb[i] & 0x80) >> 7),
                cycle,
                lp_x[i],
                lp_y[i],
                lp_y[i] << 1);
        cycle = (cycle + 1) % cycles_per_line;
    }

    fprintf(stdout,"\n");
    return 0;
}

int main(int argc, char **argv)
{
    int i;

    if(argc < 2) {
        fprintf(stderr,"Usage: %s file.prg\n", argv[0]);
        return 1;
    }

    for(i = 1; i < argc; ++i) {
        if(analyse(argv[i])) {
            return 1;
        }
    }
    return 0;
}

